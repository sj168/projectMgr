
#ifndef JPEG_ENCODER_H
#define JPEG_ENCODER_H

#include <NvJpegEncoder.h>
#include <NvVideoConverter.h>

struct JpegEncodeContext
{
    NvJPEGEncoder * jpeg_encoder;
    NvBuffer *buf_ptr;
    NvVideoConverter * video_cnvt;
    unsigned int image_width;
    unsigned int image_height;
    unsigned long max_buf_size;
    unsigned char * out_buf_ptr;
    bool save_file;
    bool show_enc_time;
    uint64_t frame_num;
    uint64_t frame_cnt;
};

class JpegEncoder
{
public:
    JpegEncoder();
    ~JpegEncoder();

    // 初始化编解码器
    bool initEncoder(unsigned int image_width, unsigned int image_height);

    /*
     * image_data_ptr：图像数据指针，图像数据必须是灰度数据
     * jpeg_enc_size：此参数用于获取压缩后的数据大小
     * image_file_name：当此指针不为空时，压缩完成后，以此为文件名写压缩数据到文件中
     * 返回值：返回压缩后的数据缓冲区的首地址，用户一定不要主动释放该指针，否则会出问题
     */
    unsigned char *jpegEncode(const unsigned char *image_data_ptr, unsigned long &jpeg_enc_size, char *image_file_name = NULL);

private:
    void preHandleGray8Buffer(NvBuffer & buffer);
    void handleGray8Buffer(const unsigned char *image_data_ptr, NvBuffer & buffer);
    JpegEncodeContext jpeg_encoder_ctxt_; // JPEG 压缩参数
};

#endif // JPEG_ENCODER_H
