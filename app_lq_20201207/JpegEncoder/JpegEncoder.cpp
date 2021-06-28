
#include <fstream>

#include "JpegEncoder.h"
#include <string.h>

JpegEncoder::JpegEncoder()
{
    jpeg_encoder_ctxt_.jpeg_encoder = NULL;
    jpeg_encoder_ctxt_.buf_ptr = NULL;
    jpeg_encoder_ctxt_.video_cnvt = NULL;
    jpeg_encoder_ctxt_.image_width = 0;
    jpeg_encoder_ctxt_.image_height = 0;
    jpeg_encoder_ctxt_.max_buf_size = 0;
    jpeg_encoder_ctxt_.out_buf_ptr = NULL;
    jpeg_encoder_ctxt_.save_file = false;
    jpeg_encoder_ctxt_.show_enc_time = false;
    jpeg_encoder_ctxt_.frame_num = 0;
    jpeg_encoder_ctxt_.frame_cnt = 0;
}

JpegEncoder::~JpegEncoder()
{
    if (jpeg_encoder_ctxt_.jpeg_encoder)
        delete jpeg_encoder_ctxt_.jpeg_encoder;

    if (NULL != jpeg_encoder_ctxt_.video_cnvt)
    {
        delete jpeg_encoder_ctxt_.video_cnvt;
        jpeg_encoder_ctxt_.video_cnvt = NULL;
    }

    if (jpeg_encoder_ctxt_.out_buf_ptr)
        delete [] jpeg_encoder_ctxt_.out_buf_ptr;

    if (NULL != jpeg_encoder_ctxt_.buf_ptr)
    {
        jpeg_encoder_ctxt_.buf_ptr->deallocateMemory();
        delete jpeg_encoder_ctxt_.buf_ptr;
    }
}

bool JpegEncoder::initEncoder(unsigned int image_width, unsigned int image_height)
{
    jpeg_encoder_ctxt_.image_width = image_width;
    jpeg_encoder_ctxt_.image_height = image_height;
    jpeg_encoder_ctxt_.max_buf_size = image_width * image_height * 3 / 2;

    if (NULL != jpeg_encoder_ctxt_.out_buf_ptr)
    {
        delete [] jpeg_encoder_ctxt_.out_buf_ptr;
        jpeg_encoder_ctxt_.out_buf_ptr = NULL;
    }

    jpeg_encoder_ctxt_.out_buf_ptr = new unsigned char[jpeg_encoder_ctxt_.max_buf_size];
    if (jpeg_encoder_ctxt_.out_buf_ptr == NULL)
        return false;
    static int cnt = 0;
    char jpegName[16] = {0};
    sprintf(jpegName, "jpegenc%d", cnt++);

    if (jpeg_encoder_ctxt_.jpeg_encoder == NULL)
    {
//        jpeg_encoder_ctxt_.jpeg_encoder = NvJPEGEncoder::createJPEGEncoder("jpegenc");
        jpeg_encoder_ctxt_.jpeg_encoder = NvJPEGEncoder::createJPEGEncoder(jpegName);
        if (jpeg_encoder_ctxt_.jpeg_encoder == NULL)
        {
            printf("Failed to create JPEGEncoder.\n");
            return false;
        }
    }

    if (jpeg_encoder_ctxt_.buf_ptr != NULL)
    {
        jpeg_encoder_ctxt_.buf_ptr->deallocateMemory();
        delete jpeg_encoder_ctxt_.buf_ptr;
        jpeg_encoder_ctxt_.buf_ptr = NULL;
    }

    jpeg_encoder_ctxt_.buf_ptr = new NvBuffer(V4L2_PIX_FMT_YUV420M, image_width,
                                              image_height, 0);
    if (NULL == jpeg_encoder_ctxt_.buf_ptr)
    {
        printf("Failed to create NvBuffer\n");
        return false;
    }

    // if (DO_STAT)
    //     jpeg_encoder_ctxt_.pJpegEncoder->enableProfiling();

    if (0 != jpeg_encoder_ctxt_.buf_ptr->allocateMemory())
    {
        printf("Failed to allocate NvBuffer memory\n");
        return false;
    }

    preHandleGray8Buffer(*jpeg_encoder_ctxt_.buf_ptr);

    // if (!initVideoConverter())
    // {
    //     printf("Failed to init video converter\n");
    //     return false;
    // }

    // return initVideoConverterCaptureBuf();

    return true;
}

void JpegEncoder::preHandleGray8Buffer(NvBuffer & buffer)
{
    unsigned int i, j;
    char * data;

    for (i = 1; i < buffer.n_planes; i++)
    {
        NvBuffer::NvBufferPlane &plane = buffer.planes[i];
        std::streamsize bytes_to_read =
            plane.fmt.bytesperpixel * plane.fmt.width;
        data = (char *) plane.data;
        plane.bytesused = 0;
        for (j = 0; j < plane.fmt.height; ++j)
        {
            memset(data, 0x80, bytes_to_read);
            // data += plane.fmt.stride;
            data += bytes_to_read;
        }
        
        plane.bytesused = plane.fmt.stride * plane.fmt.height;
    }
}

void JpegEncoder::handleGray8Buffer(const unsigned char *image_data_ptr, NvBuffer & buffer)
{
    char *data;
    NvBuffer::NvBufferPlane &plane = buffer.planes[0];
    std::streamsize bytes_to_read = plane.fmt.bytesperpixel * plane.fmt.width;
    data = (char *) plane.data;
    plane.bytesused = 0;

     for (unsigned int j = 0; j < plane.fmt.height; j++)
    {
        memcpy(data, image_data_ptr, bytes_to_read);

        data += plane.fmt.stride;
//         data += bytes_to_read;
        image_data_ptr += bytes_to_read;
    }


    plane.bytesused = plane.fmt.stride * plane.fmt.height;
}

unsigned char *JpegEncoder::jpegEncode(const unsigned char *image_data_ptr, unsigned long & jpeg_enc_size, char * image_file_name)
{
    int ret;
    unsigned long size = jpeg_encoder_ctxt_.max_buf_size;
    // unsigned char *buffer = new unsigned char[size];
    // if (!buffer)
    //     return nullptr;

    handleGray8Buffer(image_data_ptr, *jpeg_encoder_ctxt_.buf_ptr);

    // gettimeofday(&tv1, &tz);
    // https://blog.csdn.net/qq_31638535/article/details/85096887
    // std::chrono::high_resolution_clock::time_point tnow = std::chrono::high_resolution_clock::now();

    ret = jpeg_encoder_ctxt_.jpeg_encoder->encodeFromBuffer(*jpeg_encoder_ctxt_.buf_ptr, JCS_YCbCr, &jpeg_encoder_ctxt_.out_buf_ptr, size);
    if (ret == 0 && image_file_name != nullptr)
    {
        static std::ofstream outputFile;
        outputFile.open(image_file_name);

        if (outputFile.is_open())
        {
            outputFile.write((char *)jpeg_encoder_ctxt_.out_buf_ptr, size);
            outputFile.close();
        }
    }

    if (ret == 0)
    {
        jpeg_enc_size = size;
        return jpeg_encoder_ctxt_.out_buf_ptr;
    }

    jpeg_enc_size = 0;
    return nullptr;
}
