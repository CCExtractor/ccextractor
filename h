[1mdiff --git i/src/lib_ccx/ccx_decoders_isdb.c w/src/lib_ccx/ccx_decoders_isdb.c[m
[1mindex 5a54340..14444cb 100644[m
[1m--- i/src/lib_ccx/ccx_decoders_isdb.c[m
[1m+++ w/src/lib_ccx/ccx_decoders_isdb.c[m
[36m@@ -141,29 +141,77 @@[m [mtypedef uint32_t rgba;[m
 static rgba Default_clut[128] =[m
     {[m
 	// 0-7[m
[31m-	RGBA(0, 0, 0, 255), RGBA(255, 0, 0, 255), RGBA(0, 255, 0, 255), RGBA(255, 255, 0, 255),[m
[31m-	RGBA(0, 0, 255, 255), RGBA(255, 0, 255, 255), RGBA(0, 255, 255, 255), RGBA(255, 255, 255, 255),[m
[32m+[m	[32mRGBA(0, 0, 0, 255),[m
[32m+[m	[32mRGBA(255, 0, 0, 255),[m
[32m+[m	[32mRGBA(0, 255, 0, 255),[m
[32m+[m	[32mRGBA(255, 255, 0, 255),[m
[32m+[m	[32mRGBA(0, 0, 255, 255),[m
[32m+[m	[32mRGBA(255, 0, 255, 255),[m
[32m+[m	[32mRGBA(0, 255, 255, 255),[m
[32m+[m	[32mRGBA(255, 255, 255, 255),[m
 	// 8-15[m
[31m-	RGBA(0, 0, 0, 0), RGBA(170, 0, 0, 255), RGBA(0, 170, 0, 255), RGBA(170, 170, 0, 255),[m
[31m-	RGBA(0, 0, 170, 255), RGBA(170, 0, 170, 255), RGBA(0, 170, 170, 255), RGBA(170, 170, 170, 255),[m
[32m+[m	[32mRGBA(0, 0, 0, 0),[m
[32m+[m	[32mRGBA(170, 0, 0, 255),[m
[32m+[m	[32mRGBA(0, 170, 0, 255),[m
[32m+[m	[32mRGBA(170, 170, 0, 255),[m
[32m+[m	[32mRGBA(0, 0, 170, 255),[m
[32m+[m	[32mRGBA(170, 0, 170, 255),[m
[32m+[m	[32mRGBA(0, 170, 170, 255),[m
[32m+[m	[32mRGBA(170, 170, 170, 255),[m
 	// 16-23[m
[31m-	RGBA(0, 0, 85, 255), RGBA(0, 85, 0, 255), RGBA(0, 85, 85, 255), RGBA(0, 85, 170, 255),[m
[31m-	RGBA(0, 85, 255, 255), RGBA(0, 170, 85, 255), RGBA(0, 170, 255, 255), RGBA(0, 255, 85, 255),[m
[32m+[m	[32mRGBA(0, 0, 85, 255),[m
[32m+[m	[32mRGBA(0, 85, 0, 255),[m
[32m+[m	[32mRGBA(0, 85, 85, 255),[m
[32m+[m	[32mRGBA(0, 85, 170, 255),[m
[32m+[m	[32mRGBA(0, 85, 255, 255),[m
[32m+[m	[32mRGBA(0, 170, 85, 255),[m
[32m+[m	[32mRGBA(0, 170, 255, 255),[m
[32m+[m	[32mRGBA(0, 255, 85, 255),[m
 	// 24-31[m
[31m-	RGBA(0, 255, 170, 255), RGBA(85, 0, 0, 255), RGBA(85, 0, 85, 255), RGBA(85, 0, 170, 255),[m
[31m-	RGBA(85, 0, 255, 255), RGBA(85, 85, 0, 255), RGBA(85, 85, 85, 255), RGBA(85, 85, 170, 255),[m
[32m+[m	[32mRGBA(0, 255, 170, 255),[m
[32m+[m	[32mRGBA(85, 0, 0, 255),[m
[32m+[m	[32mRGBA(85, 0, 85, 255),[m
[32m+[m	[32mRGBA(85, 0, 170, 255),[m
[32m+[m	[32mRGBA(85, 0, 255, 255),[m
[32m+[m	[32mRGBA(85, 85, 0, 255),[m
[32m+[m	[32mRGBA(85, 85, 85, 255),[m
[32m+[m	[32mRGBA(85, 85, 170, 255),[m
 	// 32-39[m
[31m-	RGBA(85, 85, 255, 255), RGBA(85, 170, 0, 255), RGBA(85, 170, 85, 255), RGBA(85, 170, 170, 255),[m
[31m-	RGBA(85, 170, 255, 255), RGBA(85, 255, 0, 255), RGBA(85, 255, 85, 255), RGBA(85, 255, 170, 255),[m
[32m+[m	[32mRGBA(85, 85, 255, 255),[m
[32m+[m	[32mRGBA(85, 170, 0, 255),[m
[32m+[m	[32mRGBA(85, 170, 85, 255),[m
[32m+[m	[32mRGBA(85, 170, 170, 255),[m
[32m+[m	[32mRGBA(85, 170, 255, 255),[m
[32m+[m	[32mRGBA(85, 255, 0, 255),[m
[32m+[m	[32mRGBA(85, 255, 85, 255),[m
[32m+[m	[32mRGBA(85, 255, 170, 255),[m
 	// 40-47[m
[31m-	RGBA(85, 255, 255, 255), RGBA(170, 0, 85, 255), RGBA(170, 0, 255, 255), RGBA(170, 85, 0, 255),[m
[31m-	RGBA(170, 85, 85, 255), RGBA(170, 85, 170, 255), RGBA(170, 85, 255, 255), RGBA(170, 170, 85, 255),[m
[32m+[m	[32mRGBA(85, 255, 255, 255),[m
[32m+[m	[32mRGBA(170, 0, 85, 255),[m
[32m+[m	[32mRGBA(170, 0, 255, 255),[m
[32m+[m	[32mRGBA(170, 85, 0, 255),[m
[32m+[m	[32mRGBA(170, 85, 85, 255),[m
[32m+[m	[32mRGBA(170, 85, 170, 255),[m
[32m+[m	[32mRGBA(170, 85, 255, 255),[m
[32m+[m	[32mRGBA(170, 170, 85, 255),[m
 	// 48-55[m
[31m-	RGBA(170, 170, 255, 255), RGBA(170, 255, 0, 255), RGBA(170, 255, 85, 255), RGBA(170, 255, 170, 255),[m
[31m-	RGBA(170, 255, 255, 255), RGBA(255, 0, 85, 255), RGBA(255, 0, 170, 255), RGBA(255, 85, 0, 255),[m
[32m+[m	[32mRGBA(170, 170, 255, 255),[m
[32m+[m	[32mRGBA(170, 255, 0, 255),[m
[32m+[m	[32mRGBA(170, 255, 85, 255),[m
[32m+[m	[32mRGBA(170, 255, 170, 255),[m
[32m+[m	[32mRGBA(170, 255, 255, 255),[m
[32m+[m	[32mRGBA(255, 0, 85, 255),[m
[32m+[m	[32mRGBA(255, 0, 170, 255),[m
[32m+[m	[32mRGBA(255, 85, 0, 255),[m
 	// 56-63[m
[31m-	RGBA(255, 85, 85, 255), RGBA(255, 85, 170, 255), RGBA(255, 85, 255, 255), RGBA(255, 170, 0, 255),[m
[31m-	RGBA(255, 170, 85, 255), RGBA(255, 170, 170, 255), RGBA(255, 170, 255, 255), RGBA(255, 255, 85, 255),[m
[32m+[m	[32mRGBA(255, 85, 85, 255),[m
[32m+[m	[32mRGBA(255, 85, 170, 255),[m
[32m+[m	[32mRGBA(255, 85, 255, 255),[m
[32m+[m	[32mRGBA(255, 170, 0, 255),[m
[32m+[m	[32mRGBA(255, 170, 85, 255),[m
[32m+[m	[32mRGBA(255, 170, 170, 255),[m
[32m+[m	[32mRGBA(255, 170, 255, 255),[m
[32m+[m	[32mRGBA(255, 255, 85, 255),[m
 	// 64[m
 	RGBA(255, 255, 170, 255),[m
 	// 65-127 are calculated later.[m
[1mdiff --git i/src/lib_ccx/mp4.c w/src/lib_ccx/mp4.c[m
[1mindex 48e20e8..90509ef 100644[m
[1m--- i/src/lib_ccx/mp4.c[m
[1m+++ w/src/lib_ccx/mp4.c[m
[36m@@ -891,7 +891,7 @@[m [mint processmp4(struct lib_ccx_ctx *ctx, struct ccx_s_mp4Cfg *cfg, char *file)[m
 	if (enc_ctx)[m
 		enc_ctx->timing = dec_ctx->timing;[m
 [m
[31m-		// WARN: otherwise cea-708 will not work[m
[32m+[m	[32m// WARN: otherwise cea-708 will not work[m
 #ifndef DISABLE_RUST[m
 	ccxr_dtvcc_set_encoder(dec_ctx->dtvcc_rust, enc_ctx);[m
 #else[m
[1mdiff --git i/src/lib_ccx/utility.c w/src/lib_ccx/utility.c[m
[1mindex 88c89b2..0f52056 100644[m
[1m--- i/src/lib_ccx/utility.c[m
[1m+++ w/src/lib_ccx/utility.c[m
[36m@@ -298,7 +298,7 @@[m [mint hex_to_int(char high, char low)[m
 extern int ccxr_hex_string_to_int(const char *string, int len);[m
 int hex_string_to_int(char *string, int len)[m
 {[m
[31m-    return ccxr_hex_string_to_int(string, len);[m
[32m+[m	[32mreturn ccxr_hex_string_to_int(string, len);[m
 }[m
 #else[m
 int hex_string_to_int(char *string, int len)[m
[1mdiff --git i/src/rust/lib_ccxr/src/util/mod.rs w/src/rust/lib_ccxr/src/util/mod.rs[m
[1mindex ef0e092..ebc752e 100644[m
[1m--- i/src/rust/lib_ccxr/src/util/mod.rs[m
[1m+++ w/src/rust/lib_ccxr/src/util/mod.rs[m
[36m@@ -34,4 +34,6 @@[m [mmod tests {[m
         assert_eq!(&buffer[..test_string.len()], test_string.as_bytes());[m
         assert_eq!(buffer[test_string.len()], 0);[m
     }[m
[31m-}[m
[32m+[m[32m}[m[41m    [m
[41m+[m
[41m+[m
