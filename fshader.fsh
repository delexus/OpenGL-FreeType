uniform sampler2D qt_Texture0;
uniform vec3 textColor;

varying highp vec2 qt_TexCoord0;

void main(void)
{

    vec4 sampled = vec4(1.0, 1.0, 1.0, texture2D(qt_Texture0, qt_TexCoord0).r);
    gl_FragColor = vec4(textColor, 1.0) * sampled;
}
