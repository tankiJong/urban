
struct PSInput {
   float4 pos: SV_POSITION;
   float4 color: COLOR;
};

static const float4 positions[3] = {
   float4(.5, .0, .5, 1.f),
   float4(.0, .5, .5, 1.f),
   float4(.0, .0, .0, 1.f),
};

static const float4 colors[3] = {
   float4(1., .0, .0, 1.f),
   float4(.0, 1., .0, 1.f),
   float4(.0, .0, 1., 1.f),
};

PSInput main( float4 pos : POSITION, uint Vid: VertexID )
{
   PSInput output;
   output.pos = positions[Vid];
   output.color = colors[Vid];
	return output;
}