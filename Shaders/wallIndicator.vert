attribute vec3 position;

uniform mat4 modelviewProjection;
uniform vec3 gridPosition;

void main()
{
    gl_Position = modelviewProjection * vec4(position + gridPosition, 1.0);
}
