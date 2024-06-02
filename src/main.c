#include <SDL.h>
#include <math.h>
#include <errno.h>

#define W 128
#define H 128

#ifdef DEBUG
    #define check(x)\
    if (!(x)) {\
        if (errno) {\
            flockfile(stderr);\
            fprintf(stderr, "%s:%d " , __FILE__, __LINE__);\
            perror(NULL);\
            funlockfile(stderr);\
        }\
        errno = 0;\
        goto error;\
    }
#else
    #define check(x) if (!(x)) { errno = 0; goto error; }
#endif

typedef enum {
    PROGRAM_LIMIT = 500,
} limits_enum;

typedef enum {
    LT,
    GT,
    EQ,
    ER,
    FLAGS_COUNT,
} flags_enum;

typedef enum {
    R0 = 0,
    R1,
    R2,
    R3,
    R4,
    R5,
    R6,
    R7,

    I0,
    I1,
    I2,
    I3,

    O0,
    O1,

    T0,

    REGISTERS_COUNT,
} registers_enum;

const char *registers_enum_strings[] = {
    "R0",
    "R1",
    "R2",
    "R3",
    "R4",
    "R5",
    "R6",
    "R7",

    "I0",
    "I1",
    "I2",
    "I3",

    "O0",
    "O1",

    "T0",

    "REGISTERS_COUNT",
};

typedef enum {
    PUTI = 0,
    PUTC,
    INC,
    DEC,

    INCEQ,
    DECEQ,

    ADD,
    SUB,
    MUL,
    PIXEL,
    SYNC,
    STORE,
    MOVE,
    HALT,
    SLEEP,
    LABEL,
    JMP,
    JMPEQ,
    JMPNE,
    JMPGT,
    JMPLT,
    CMP,
    XOR,
    BUTTON,
    RESTART,
    CLS,

    OPS_COUNT,
} ops_enum;

const char *ops_enum_strings[] = {
    "PUTI",
    "PUTC",
    "INC",
    "DEC",

    "INCEQ",
    "DECEQ",

    "ADD",
    "SUB",
    "MUL",
    "PIXEL",
    "SYNC",
    "STORE",
    "MOVE",
    "HALT",
    "SLEEP",
    "LABEL",
    "JMP",
    "JMPEQ",
    "JMPNE",
    "JMPGT",
    "JMPLT",
    "CMP",
    "XOR",
    "BUTTON",
    "RESTART",
    "CLS",

    "OPS_COUNT",
};

int code[PROGRAM_LIMIT];
int code_length;
#define CODE_AT_PC ((int)code[pc])

typedef struct program {
    char *asmcode;
    size_t asmcode_len;
    int *bytecode;
    size_t bytecode_len;
    int labels[10];
} program;

program user_program;

int flags[FLAGS_COUNT];
int registers[REGISTERS_COUNT];

typedef struct software_texture
{
    SDL_Surface *surface;
    int w, h;
    Uint32 *pixels;
} software_texture;

static software_texture cpu_texture;
static SDL_Texture *gpu_texture;
static SDL_Renderer *renderer;

static int done()
{
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
        case SDL_QUIT:
            return 1;
        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_ESCAPE:
                return 1;
            }
            break;
        }
    }
    return 0;
}

int read_code(const char *path, program *user_program)
{
    size_t len = -1;
    user_program->asmcode = NULL;
    user_program->asmcode_len = -1;
    FILE *fp = fopen(path, "r");

    if (fp != NULL) {
        if (fseek(fp, 0L, SEEK_END) == 0) {
            long bufsize = ftell(fp);
            if (bufsize == -1) { /* Error */ }
            user_program->asmcode = malloc(sizeof(char) * (bufsize + 1));
            user_program->bytecode = malloc(sizeof(int) * (bufsize + 1));
            if (fseek(fp, 0L, SEEK_SET) != 0) { /* Error */ }
            len = fread(user_program->asmcode, sizeof(char), bufsize, fp);
            if (len == 0) {
                fputs("Error reading file", stderr);
            } else {
                user_program->asmcode[++len] = '\0'; /* Just to be safe. */
            }
            user_program->asmcode_len = len;
        }
        fclose(fp);
    }
    return user_program->asmcode_len;
}

static char *skip_space(char *p, char *end)
{
    for(;p < end && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); ++p);
    return p <= end ? p : NULL;
}

static char *next_space(char *p, char *end)
{
    for(;p < end && !(*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n'); ++p);
    return p <= end ? p : NULL;
}

static char *skip_comma(char *p, char *end)
{
    for(;p < end && *p == ','; ++p);
    return p <= end ? p : NULL;
}

static char *next_eol(char *p, char *end)
{
    for(;p < end && *p != '\n'; ++p);
    return p <= end ? p : NULL;
}

int is_opcode(char *p, char *end, ops_enum opcode)
{
    const char *opcode_string = ops_enum_strings[opcode];
    size_t n = strlen(opcode_string);

    char *t = next_space(p, end);
    int oplen = t - p;

    if (n != oplen) return 0; 
    if (strncmp(p, opcode_string, oplen) == 0) return 1;
    return 0;
}

int next_opcode(char *p, char *end)
{
    for (int i = 0; i < OPS_COUNT; i++) {
        if (is_opcode(p, end, i)) {
            return i;
        }
    }
    return -1;
}

registers_enum get_register(char *t, char *end)
{
    for (int i = 0; i < REGISTERS_COUNT; i++) {

        size_t slen = strlen(registers_enum_strings[i]);
        if (slen < (end - t)) {
            if (memcmp(t, registers_enum_strings[i], slen) == 0) {
                return i;
            }
        }
    }
    return -1;
}

void program_set_bytecode(program *p, int code)
{
    p->bytecode[p->bytecode_len++] = code;
}

void parse_code(program *user_program)
{
    char *p = user_program->asmcode;
    char *end = p + user_program->asmcode_len;
    char *t;
    char *t_end;
    int v;
    char buf[64];
    ops_enum opcode;
    registers_enum r;
    user_program->bytecode_len = 0;
    for (;;) {

        memset(buf, 0, sizeof(buf));
        check((t = skip_space(p, end)))
        opcode = next_opcode(t, end);
        if (opcode == -1) break;
        switch(opcode) {
            case STORE:
            {
                // Write a store opcode to the program
                program_set_bytecode(user_program, opcode);

                // Get next token, which should be a register
                t += strlen(ops_enum_strings[opcode]);
                check((t = skip_space(t, end)));

                // Convert text to register enum
                check((r = get_register(t, end)) != -1);

                // Write register enum to the program
                program_set_bytecode(user_program, r);

                // Get next token, which should be an int
                t += 2; // Regster string length is 2
                check((t = skip_space(t, end)));
                check((t = skip_comma(t, end)));
                check((t = skip_space(t, end)));
                check((t_end = next_eol(t, end)));
                check(memcpy(buf, t, t_end - t));

                // Convert text to int
                v = strtol(buf, NULL, 0);
                program_set_bytecode(user_program, v);

                p = t_end;
            }
            break;
            case MOVE:
            {
                // Write a move opcode to the program
                program_set_bytecode(user_program, opcode);

                // Get next token, which should be a register
                t += strlen(ops_enum_strings[opcode]);
                check((t = skip_space(t, end)));

                // Convert text to register enum
                check((r = get_register(t, end)) != -1);

                // Write register enum to the program
                program_set_bytecode(user_program, r);

                // Get next token, which should be
                t += 2; // Regster string length is 2

                check((t = skip_space(t, end)));
                check((t = skip_comma(t, end)));
                check((t = skip_space(t, end)));
                check((t_end = next_eol(t, end)));
                check((r = get_register(t, end)) != -1);
                program_set_bytecode(user_program, r);

                p = t_end;

            }
            break;
            case INC:
            case INCEQ:
            case DEC:
            case DECEQ:
            {
                program_set_bytecode(user_program, opcode);

                // Get next token, which should be a register
                t += strlen(ops_enum_strings[opcode]);
                check((t = skip_space(t, end)));
                check((r = get_register(t, end)) != -1);
                t+=2;

                program_set_bytecode(user_program, r);
                check((t_end = next_eol(t, end)));

                p = t_end;
            }
            break;
            case LABEL:
            case JMP:
            case JMPLT:
            case JMPGT:
            case JMPNE:
            case JMPEQ:
            {
                // Write opcode to the program
                program_set_bytecode(user_program, opcode);

                // Get next token, which should be a number
                t += strlen(ops_enum_strings[opcode]);
                check((t = skip_space(t, end)));
                check((t_end = next_eol(t, end)));
                check(memcpy(buf, t, t_end - t));

                v = strtol(buf, NULL, 0);
                program_set_bytecode(user_program, v);

                p = t_end;

                if (opcode == LABEL) {
                    user_program->labels[v] = user_program->bytecode_len - 1;
                }

            }
            break;
            case CLS:
            case RESTART:
            case CMP:
            case ADD:
            case SUB:
            case MUL:
            case XOR:
            case PIXEL:
            case SYNC:
            case HALT:
            case SLEEP:
            case BUTTON:
            case PUTC:
            case PUTI:
            {
                // Write opcode to the program
                program_set_bytecode(user_program, opcode);
                t += strlen(ops_enum_strings[opcode]);
                p = t;
                break;
            }
            case OPS_COUNT:
            {
                break;
            }
        }
    }

    return;

error:
    exit(100);

}

void regvm_init()
{
    memset(registers, 0, sizeof(registers));
    memset(code, 0, sizeof(code));

    code_length = user_program.bytecode_len;
    memcpy(&code, user_program.bytecode, user_program.bytecode_len * sizeof(int));
}

static void tick(long millis)
{
    (void)millis;

    // Handle mouse
    int mouse_x, mouse_y;
    int mouse_state = SDL_GetRelativeMouseState(&mouse_x, &mouse_y);

    if (mouse_state & SDL_BUTTON_LMASK) {
    } else { }
    if (mouse_state & SDL_BUTTON_RMASK) {
    } else { }

}

static int button_pressed(int button)
{
    // Get keyboard state
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    switch(button) {
        case 0:
            return keys[SDL_SCANCODE_A] || keys[SDL_SCANCODE_LEFT];
            break;
        case 1:
            return keys[SDL_SCANCODE_D] || keys[SDL_SCANCODE_RIGHT];
            break;
        default:
            return 0;
    }
}

static void sync()
{
    // Tick
    static Uint32 current_millis;
    static Uint32 last_millis = 0;

    current_millis = SDL_GetTicks();
    if (current_millis % 1000 == 0) {
        //printf("%d\n", current_millis - last_millis);
    }
    tick(current_millis);
    while (current_millis < last_millis + 16) {
        current_millis = SDL_GetTicks();
        SDL_Delay(1);
    }
    last_millis = current_millis;

    // Update the gpu texture
    SDL_Rect r = {.w = cpu_texture.w,.h = cpu_texture.h,.x = 0,.y = 0 };
    SDL_UpdateTexture(gpu_texture, &r, cpu_texture.pixels,
		      cpu_texture.w * sizeof(Uint32));
    SDL_RenderCopy(renderer, gpu_texture, NULL, NULL);

    // Do hardware drawing
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
   
    // Present
    SDL_RenderPresent(renderer);

}

void regvm_program_loop(){

    register int op = 0;
    register int i0 = 0;
    register int i1 = 0;
    for(register int pc = 0; pc < code_length && !done(); pc++) {
        op = code[pc];
        registers[T0] = SDL_GetTicks();
        switch(op){

            case SYNC:
                sync();
                continue;

            case STORE:

                // data from code
                pc++;
                i0 = code[pc];

                // data from code
                pc++;
                i1 = code[pc];

                registers[i0] = i1;
                continue;

            case MOVE:
                // data from code
                pc++;
                i0 = code[pc];

                // data from code
                pc++;
                i1 = code[pc];

                registers[i0] = registers[i1];

                continue;

            case PUTI:
                pc++;
                printf("%d", (int)registers[I0]);
                fflush(stdout);
                continue;

            case PUTC:
                pc++;
                printf("%c", (char)registers[I0]);
                fflush(stdout);
                continue;

            case ADD:
                registers[O0] = registers[I0] + registers[I1];
                continue;

            case SUB:
                registers[O0] = registers[I0] - registers[I1];
                continue;

            case MUL:
                registers[O0] = registers[I0] * registers[I1];
                continue;

            case INC:
                pc++;
                registers[CODE_AT_PC]++;
                continue;

            case DEC:
                pc++;
                registers[CODE_AT_PC]--;
                continue;

            case INCEQ:
                pc++;
                if (flags[EQ]) {
                    registers[CODE_AT_PC]++;
                }
                continue;

            case DECEQ:
                pc++;
                if (flags[EQ]) {
                    registers[CODE_AT_PC]--;
                }
                continue;

            case XOR:
                registers[O0] = (int)registers[I0] ^ (int)registers[I1];
                continue;

            case BUTTON:
                registers[O0] = button_pressed((int)registers[I0]);
                continue;

            case SLEEP:
                SDL_Delay(registers[I0]);
                continue;

            case PIXEL:
                {
                    int x = registers[I0];
                    int y = registers[I1];
                    int c = registers[I2];
                    if (x < 0 || y < 0) continue;
                    if (x >= cpu_texture.w || y >= cpu_texture.h) continue;;

                    cpu_texture.pixels[y * W + x] = c;
                }
                continue;

            case HALT:
                pc = code_length;
                continue;

            case RESTART:
                pc = -1;
                continue;

            case LABEL:
                pc++;
                continue;

            case CMP:
                i0 = registers[I0];
                i1 = registers[I1];
                flags[EQ] = i0 == i1;
                flags[LT] = i0 < i1;
                flags[GT] = i0 > i1;
                flags[ER] = 0;
                continue;

            case JMPEQ:
                pc++;
                i0 = code[pc];
                if (flags[EQ]) {
                    pc = user_program.labels[i0];
                }
                continue;

            case JMPNE:
                pc++;
                i0 = code[pc];
                if (!flags[EQ]) {
                    pc = user_program.labels[i0];
                }
                continue;

            case JMPGT:
                pc++;
                i0 = code[pc];
                if (flags[GT]) {
                    pc = user_program.labels[i0];
                }
                continue;

            case JMPLT:
                pc++;
                i0 = code[pc];
                if (flags[LT]) {
                    pc = user_program.labels[i0];
                }
                continue;

            case JMP:
                pc++;
                i0 = code[pc];
                pc = user_program.labels[i0];
                continue;
            case CLS:
                memset(cpu_texture.pixels, 0x333333, W * H * sizeof(Uint32));
                continue;
        }
    }
}
    
int main(int argc, char *argv[])
{

    if (argc < 2) return 0;
    const char *code_path_arg = argv[1];

    // sdl init
    SDL_Init(SDL_INIT_EVERYTHING);

    // no filtering
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

    // create window
    SDL_Window *window = SDL_CreateWindow("pixels", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
            W * 6, H * 6, SDL_WINDOW_RESIZABLE);

    // create renderer
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);

    // create gpu texture
    gpu_texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_STREAMING, W, H);
    SDL_RenderSetLogicalSize(renderer, W, H);
    SDL_SetRenderTarget(renderer, gpu_texture);
    SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
    SDL_RenderClear(renderer);

    // create cpu texture
#if SDL_BYTEORDER == SDL_BIG_ENDIAN
#define rmask 0xff000000
#define gmask 0x00ff0000
#define bmask 0x0000ff00
#define amask 0x000000ff
#else
#define rmask 0x000000ff
#define gmask 0x0000ff00
#define bmask 0x00ff0000
#define amask 0xff000000
#endif
    SDL_Surface *surface = SDL_CreateRGBSurface(0, W, H, 32, rmask, gmask, bmask, amask);
    cpu_texture.surface = surface;
    cpu_texture.w = surface->w;
    cpu_texture.h = surface->h;
    cpu_texture.pixels = (Uint32 *) surface->pixels;
    SDL_SetSurfaceBlendMode(cpu_texture.surface, SDL_BLENDMODE_NONE);

    if(read_code(code_path_arg, &user_program) > 0) {
        parse_code(&user_program);

        regvm_init();
        regvm_program_loop();
    }

    // Cleanup
    return 0;
}

