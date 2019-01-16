shader "downscale4x4LogLum" {
	glsl_vp {
		$include "clipQuad.vp"
	}

	glsl_fp {
		in vec2 v2f_texCoord;

		out vec4 o_fragColor : FRAG_COLOR;

		#ifdef LOGLUV_HDR
		$include "logluv.glsl"
		#endif

		#if 1// BILINEAR_FP16
		#define SAMPLES 4
		#else
		#define SAMPLES 16
		#endif

		uniform sampler2D tex0;
		uniform vec2 sampleOffsets[SAMPLES];

		const vec3 lumVector = vec3(0.2125, 0.7154, 0.0721);

		void main() {
			float sumLogLum = 0.0;
			float luminance;

			for (int i = 0; i < SAMPLES; i++) {
                vec4 color = tex2D(tex0, v2f_texCoord.st + sampleOffsets[i]).rgba;

		#ifdef LOGLUV_HDR
				luminance = max(dot(decodeLogLuv(color).rgb, lumVector), 0.0001);
		#else
                luminance = max(dot(color.rgb, lumVector), 0.0001);
		#endif
				sumLogLum += log(luminance);
			}

            float avgLogLum = sumLogLum / float(SAMPLES);

            o_fragColor = vec4(avgLogLum, avgLogLum, avgLogLum, 1.0);
		}
	}
}
