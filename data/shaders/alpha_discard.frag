// alpha_discard.frag
uniform sampler2D texture;

in vec2 fragTexCoord;
out vec4 fragColor;

void main()
{
    vec4 texColor = texture(texture, fragTexCoord);

    if (texColor.a <= 0.5)
        discard;

    fragColor = texColor;
}
