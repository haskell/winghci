#define rgb(r,g,b) ((r) | ((g) << 8) | ((b) << 16))

#define BLACK	rgb(0,0,0)
#define BLUE	rgb(0,0,175)
#define GREEN	rgb(0,135,0)
#define CYAN	rgb(0,175,175)
#define RED		rgb(175,0,0)
#define MAGENTA	rgb(150,0,150)
#define YELLOW	rgb(175,175,0)
#define WHITE	rgb(255,255,255)

#define PROMPT_COLOR BLUE
#define STDOUT_COLOR BLACK
#define STDERR_COLOR RED