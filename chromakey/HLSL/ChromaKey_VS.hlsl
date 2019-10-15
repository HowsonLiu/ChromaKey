#include "ChromaKey.hlsli"

VertData main(VertData v_in)
{
    VertData vert_out;
    vert_out.pos = v_in.pos;
    vert_out.uv = v_in.uv;
    return vert_out;
}