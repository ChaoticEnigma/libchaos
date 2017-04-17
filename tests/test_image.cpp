#include "tests.h"
#include "zimage.h"
#include "zfile.h"

namespace LibChaosTest {

#define GEN_BMP         "gen.bmp"
#define GEN_PPM         "gen.ppm"
#define GEN_PNG         "gen.png"
#define GEN_INVERT_PNG  "gen_invert.png"
#define GEN_16_PNG      "gen_16.png"

#define DICE_WEBP       "dice.webp"
#define DICE_PNG        "dice.png"

#define TREE_JPEG       "tree.jpg"
#define TREE_PNG        "tree.png"

#define TESTPNG_DIR     "testpng/png"
#define PNGSUITE_DIR    "pngsuite"
#define TESTJPEG_DIR    "testjpeg/imagetestsuite/jpg"

ZImage gen_image(){
    zu64 width = 1920;
    zu64 height = 1080;
    LOG("Creating " << width << "x" << height << " 8-bit color image");
    ZImage image1(width, height, ZImage::RGB24);
    image1.newData();

    for(zu64 i = 0; i < image1.pixels(); ++i){
        image1.pixelAt(i)[0] = 0x33;
        image1.pixelAt(i)[1] = 0xCC;
        image1.pixelAt(i)[2] = 0xFF;
    }

    zu64 o = (width - (height+200)) / 2;
    for(zu64 y = 0; y < image1.height(); y++){
        for(zu64 x = 0; x < 200; ++x){
            image1.pixelAt(x+o, y)[0] = 0xFF;
            image1.pixelAt(x+o, y)[1] = 0xAA;
            image1.pixelAt(x+o, y)[2] = 0x00;
        }
        ++o;
    }

    return image1;
}

void encode_bmp(){
    ZImage image1 = gen_image();
    ZBinary out1;
    TASSERT(image1.setFormat(ZImage::BMP));
    TASSERT(image1.encodeFormat(out1));
    TASSERT(ZFile::writeBinary(GEN_BMP, out1) == out1.size());
}

void encode_ppm(){
    ZImage image1 = gen_image();
    ZBinary out1;
    TASSERT(image1.setFormat(ZImage::PPM));
    TASSERT(image1.encodeFormat(out1));
    TASSERT(ZFile::writeBinary(GEN_PPM, out1) == out1.size());
}

void encode_png(){
    ZImage image1 = gen_image();
    ZBinary out1;
    TASSERT(image1.setFormat(ZImage::PNG));
    TASSERT(image1.encodeFormat(out1));
    TASSERT(ZFile::writeBinary(GEN_PNG, out1) == out1.size());
}

void invert_png(){
    ZBinary in1;
    TASSERT(ZFile::readBinary(GEN_PNG, in1));

    ZImage imagein1(in1);
    LOG("Invert image: " << (int)imagein1.getFormat() << " " << imagein1.width() << "x" << imagein1.height());
    for(zu64 i = 0; i < imagein1.pixels(); ++i){
        imagein1.pixelAt(i)[0] = 0xFF - imagein1.pixelAt(i)[0];
        imagein1.pixelAt(i)[1] = 0xFF - imagein1.pixelAt(i)[1];
        imagein1.pixelAt(i)[2] = 0xFF - imagein1.pixelAt(i)[2];
    }

    ZBinary out1;
    imagein1.encodeFormat(out1);
    TASSERT(ZFile::writeBinary(GEN_INVERT_PNG, out1) == out1.size());
}

// 16-bit
void encode_16bit(){
    zu64 width = 1920;
    zu64 height = 1080;
    LOG("Creating " << width << "x" << height << " 16-bit color image");
    ZImage image2(width, height, ZImage::RGB48);
    image2.newData();

    for(zu64 i = 0; i < image2.pixels(); ++i){
        ((zu16 *)image2.pixelAt(i))[0] = 0x3333;
        ((zu16 *)image2.pixelAt(i))[1] = 0xCCCC;
        ((zu16 *)image2.pixelAt(i))[2] = 0xFFFF;
    }

    zu64 o = (width - (height+200)) / 2;
    for(zu64 y = 0; y < image2.height(); y++){
        for(zu64 x = 0; x < 200; ++x){
            ((zu16 *)image2.pixelAt(x+o, y))[0] = 0xFFFF;
            ((zu16 *)image2.pixelAt(x+o, y))[1] = 0xAAAA;
            ((zu16 *)image2.pixelAt(x+o, y))[2] = 0x0000;
        }
        ++o;
    }

    ZBinary out2;
    image2.setFormat(ZImage::PNG);
    image2.encodeFormat(out2);
    TASSERT(ZFile::writeBinary(GEN_16_PNG, out2) == out2.size());
}

// Convert Transparent WebP to PNG
void convert_webp_png(){
    ZBinary bin3i;
    TASSERT(ZFile::readBinary(DICE_WEBP, bin3i));
    ZImage image3(bin3i);
    TASSERT(image3.setFormat(ZImage::PNG));
    ZBinary bin3o;
    TASSERT(image3.encodeFormat(bin3o));
    TASSERT(ZFile::writeBinary(DICE_PNG, bin3o) == bin3o.size());
}

// Convert JPEG to PNG
void convert_jpeg_png(){
    ZBinary bin4i;
    TASSERT(ZFile::readBinary(TREE_JPEG, bin4i));
    ZImage image4(bin4i);
    TASSERT(image4.setFormat(ZImage::PNG));
    ZBinary bin4o;
    TASSERT(image4.encodeFormat(bin4o));
    TASSERT(ZFile::writeBinary(TREE_PNG, bin4o) == bin4o.size());
}

// JPEG Decode Test
void decode_jpeg(){
    ZArray<ZPath> list1 = ZFile::listFiles(TESTJPEG_DIR);
    for(zu64 i = 0; i < list1.size(); ++i){
        ZPath jfile = list1[i];
        LOG(i+1 << ": Decode " << jfile);
        ZBinary jin;
        TASSERT(ZFile::readBinary(jfile, jin));
        ZImage jimage(jin);
        LOG("Image Size: " << jimage.size());
        if(jimage.size()){
            TASSERT(jimage.setFormat(ZImage::BMP));
            ZBinary jout;
            TASSERT(jimage.encodeFormat(jout));
            jfile.last().append(".bmp");
            TASSERT(ZFile::writeBinary(jfile, jout) == jout.size());
        }
    }
}

// PNG Decode Test
void decode_png(){
    ZArray<ZPath> list2 = ZFile::listFiles(TESTPNG_DIR);
    for(zu64 i = 0; i < list2.size(); ++i){
        ZPath pfile = list2[i];
        LOG(i+1 << ": Decode " << pfile);
        ZBinary pin;
        TASSERT(ZFile::readBinary(pfile, pin));
        ZImage pimage(pin);
        LOG("Image Size: " << pimage.size());
        if(pimage.size()){
            TASSERT(pimage.setFormat(ZImage::BMP));
            ZBinary pout;
            TASSERT(pimage.encodeFormat(pout));
            pfile.last().append(".bmp");
            TASSERT(ZFile::writeBinary(pfile, pout) == pout.size());
        }
    }
}

ZArray<Test> image_tests(){
    return {
        { "encode-bmp",         encode_bmp,         true, {} },
        { "encode-ppm",         encode_ppm,         true, {} },
#ifdef LIBCHAOS_HAS_PNG
        { "encode-png",         encode_png,         true, {} },
        { "invert-png",         invert_png,         true, { "encode-png" } },
#endif
//        { "encode-jpeg",        encode_jpeg,        ZImage::isFormatSupported(ZImage::JPEG), {} },
//        { "encode-webp",        encode_webp,        ZImage::isFormatSupported(ZImage::WEBP), {} },
//        { "encode-16bit",       encode_16bit,       true, {} },
#if defined(LIBCHAOS_HAS_PNG) && defined(LIBCHAOS_HAS_WEBP)
        { "convert-webp-png",   convert_webp_png,   false, {} },
        { "convert-jpeg-png",   convert_jpeg_png,   false, {} },
#endif
#ifdef LIBCHAOS_HAS_JPEG
        { "decode-jpeg",        decode_jpeg,        false, {} },
#endif
#ifdef LIBCHAOS_HAS_PNG
        { "decode-png",         decode_png,         false, {} },
#endif
    };
}

}
