uniform sampler2D currentTexture;
uniform sampler2D distortionMapTexture;
uniform sampler2D physicsTexture;

uniform float time;
uniform float distortionFactor;


vec4 distort(float amount)
{
  vec2 distortionMapCoordinate = gl_TexCoord[0].xy;
  distortionMapCoordinate.x -= time;

  vec4 distortionMapValue = texture2D(distortionMapTexture, distortionMapCoordinate);

  vec2 distortionPositionOffset = distortionMapValue.xy;
  distortionPositionOffset -= vec2(0.5f,  0.5f);
  distortionPositionOffset *= 2.0f;
  distortionPositionOffset *= distortionFactor * amount;

  // enable for hot air (decrease the higher up)
  // distortionPositionOffset *= (1.0f - gl_TexCoord[0].t);

  vec2 distortedTextureCoordinate = gl_TexCoord[0].st + distortionPositionOffset;
  return gl_Color * texture2D(currentTexture, distortedTextureCoordinate);
}


// water: 007f7f, 0.0f, 0.5f, 0.5f
// solid: 49146a

void main()
{
  vec4 effectColor = texture2D(physicsTexture, gl_TexCoord[0].xy);

  // debug
  // gl_FragColor = effectColor;

  // the check below is not great. ideas?
  if (effectColor.r == 0.0 && effectColor.g > 0.01 && effectColor.b > 0.01)
  {
    vec4 originalColor = texture2D(currentTexture, gl_TexCoord[0].xy);
    gl_FragColor = distort(effectColor.g);
  }
  else
  {
    vec4 ambient = vec4(0.0, 0.0, 0.0, 1.0);
    gl_FragColor = mix(ambient, gl_Color, 1.0f) * texture2D(currentTexture, gl_TexCoord[0].xy);
  }
}
