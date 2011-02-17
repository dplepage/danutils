#define W {{ W }}
#define H {{ H }}
#define N {{ W*H }}
#define PI {{ scipy.pi }}

// The following jinja2 line registers the texture with the module and emits:
// texture <float4, 2> normal;
{{ reg.texture('normal', 'float4', (N1, N2)) }}
{{ reg.texture('mask', 'unsigned char', (N1, N2)) }}
{{ reg.texture('weights', 'float', (N1, N2, 2)) }}

// The following line expands to
// float[N1*N2] data;
{{ reg.array('data', 'float', (N1, N2)) }}

{{
reg.function(
    name = "setone",
    sig = "P",
    block = (512, 1, 1),
    grid = (ceil(N1*N2/512), 1),
    texnames = ['mask'],
    doc = ("Set every entry in the array to 1"
           ""
           "Entries blocked by the mask will be ignored.")
)
}}
__global__ void setone(float* target) {
    int pixel = (blockIdx.x + gridDim.x*blockIdx.y) * blockDim.x + threadIdx.x;
    int j = pixel%W;
    int i = pixel/W;
    if (i < H) {
        if (maskI(j,i) != 0) {
            outI(i,j) = 1;
        } else {
            outI(i,j) = 0;
        }
    }
}
