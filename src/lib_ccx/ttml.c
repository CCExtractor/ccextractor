#include "ttml.h"
#include "ccx_encoders_common.h"
#include "utility.h"
#include "ccx_common_timing.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

// Initialize TTML encoder context
void ttml_init(struct ttml_ctx *ctx, struct encoder_ctx *parent)
{
    ctx->parent = parent;
    ctx->wrote_header = 0;
    ctx->last_start_time = 0;
    ctx->last_end_time = 0;
}

// Write TTML XML header
int ttml_write_header(struct ttml_ctx *ctx)
{
    if (ctx->wrote_header)
        return 0;
    
    const char *header = 
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        "<tt xmlns=\"http://www.w3.org/ns/ttml\"\n"
        "    xmlns:tts=\"http://www.w3.org/ns/ttml#styling\">\n"
        "  <head>\n"
        "    <styling>\n"
        "      <style xml:id=\"defaultStyle\" tts:fontFamily=\"monospace\" "
        "tts:fontSize=\"1c\" tts:color=\"white\"/>\n"
        "    </styling>\n"
        "  </head>\n"
        "  <body>\n"
        "    <div>\n";
    
    write(ctx->parent->out->fh, header, strlen(header));
    ctx->wrote_header = 1;
    return 0;
}

// Write TTML footer
int ttml_write_footer(struct ttml_ctx *ctx)
{
    const char *footer = 
        "    </div>\n"
        "  </body>\n"
        "</tt>\n";
    
    write(ctx->parent->out->fh, footer, strlen(footer));
    return 0;
}

// Convert timestamp to TTML format (HH:MM:SS.mmm)
static void timestamp_to_ttml(int64_t timestamp, char *buffer, size_t bufsize)
{
    int hours = (int)(timestamp / 3600000);
    int minutes = (int)((timestamp % 3600000) / 60000);
    int seconds = (int)((timestamp % 60000) / 1000);
    int milliseconds = (int)(timestamp % 1000);
    
    snprintf(buffer, bufsize, "%02d:%02d:%02d.%03d", 
             hours, minutes, seconds, milliseconds);
}

// Write a single subtitle entry in TTML format
int ttml_write_subtitle(struct ttml_ctx *ctx, struct cc_subtitle *sub)
{
    if (!ctx->wrote_header)
        ttml_write_header(ctx);
    
    char start_time[32];
    char end_time[32];
    
    timestamp_to_ttml(sub->start_time, start_time, sizeof(start_time));
    timestamp_to_ttml(sub->end_time, end_time, sizeof(end_time));
    
    // Write opening tag
    char buffer[256];
    snprintf(buffer, sizeof(buffer),
             "      <p begin=\"%s\" end=\"%s\" style=\"defaultStyle\">\n",
             start_time, end_time);
    write(ctx->parent->out->fh, buffer, strlen(buffer));
    
    // Write subtitle text (with proper XML escaping)
    for (int i = 0; i < sub->nb_data; i++) {
        unsigned char c = sub->data[i];
        
        if (c == '<')
            write(ctx->parent->out->fh, "&lt;", 4);
        else if (c == '>')
            write(ctx->parent->out->fh, "&gt;", 4);
        else if (c == '&')
            write(ctx->parent->out->fh, "&amp;", 5);
        else if (c == '"')
            write(ctx->parent->out->fh, "&quot;", 6);
        else if (c == '\n')
            write(ctx->parent->out->fh, "<br/>", 5);
        else
            write(ctx->parent->out->fh, &c, 1);
    }
    
    // Write closing tag
    const char *close_tag = "\n      </p>\n";
    write(ctx->parent->out->fh, close_tag, strlen(close_tag));
    
    return 0;
}

// Main write function called by encoder framework
int write_ttml(struct cc_subtitle *sub, struct encoder_ctx *context)
{
    struct ttml_ctx *ctx = (struct ttml_ctx *)context->private_data;
    
    if (!ctx) {
        ctx = (struct ttml_ctx *)malloc(sizeof(struct ttml_ctx));
        if (!ctx)
            return -1;
        ttml_init(ctx, context);
        context->private_data = ctx;
    }
    
    return ttml_write_subtitle(ctx, sub);
}
