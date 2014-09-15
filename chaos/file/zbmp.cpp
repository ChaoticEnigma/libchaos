#include "zbmp.h"
#include "zfile.h"

namespace LibChaos {

struct BitmapFileHeader {
    unsigned short bfType       : 16;
    unsigned int bfSize         : 32;
    unsigned short bfReserved1  : 16;
    unsigned short bfReserved2  : 16;
    unsigned int bfOffBits      : 32;
};

struct BitmapInfoHeader {
    unsigned int biSize             : 32;
    unsigned int biWidth            : 32;
    unsigned int biHeight           : 32;
    unsigned short biPlanes         : 16;
    unsigned short biBitCount       : 16;
    unsigned int biCompression      : 32;
    unsigned int biSizeImage        : 32;
    unsigned int biXPelsPerMeter    : 32;
    unsigned int biYPelsPerMeter    : 32;
    unsigned int biClrUsed          : 32;
    unsigned int biClrImportant     : 32;
};

void readFileHeader(const unsigned char *start, BitmapFileHeader *fileh){
    fileh->bfType       = *(const unsigned short*)(&start[0]);
    fileh->bfSize       = *(const unsigned int*)  (&start[2]);
    fileh->bfReserved1  = *(const unsigned short*)(&start[6]);
    fileh->bfReserved2  = *(const unsigned short*)(&start[8]);
    fileh->bfOffBits    = *(const unsigned int*)  (&start[10]);
}
void readInfoHeader(const unsigned char *start, BitmapInfoHeader *infoh){
    infoh->biSize           = *(const unsigned int*)  (&start[0]);
    infoh->biWidth          = *(const unsigned int*)  (&start[4]);
    infoh->biHeight         = *(const unsigned int*)  (&start[8]);
    infoh->biPlanes         = *(const unsigned short*)(&start[12]);
    infoh->biBitCount       = *(const unsigned short*)(&start[14]);
    infoh->biCompression    = *(const unsigned int*)  (&start[16]);
    infoh->biSizeImage      = *(const unsigned int*)  (&start[20]);
    infoh->biXPelsPerMeter  = *(const unsigned int*)  (&start[24]);
    infoh->biYPelsPerMeter  = *(const unsigned int*)  (&start[28]);
    infoh->biClrUsed        = *(const unsigned int*)  (&start[32]);
    infoh->biClrImportant   = *(const unsigned int*)  (&start[36]);
}

ZBinary writeFileHeader(const BitmapFileHeader *fileh){
    ZBinary out;
    out.fill(0, 14);
    *(unsigned short*)(&out.raw()[0])   = fileh->bfType;
    *(unsigned int*)  (&out.raw()[2])   = fileh->bfSize;
    *(unsigned short*)(&out.raw()[6])   = fileh->bfReserved1;
    *(unsigned short*)(&out.raw()[8])   = fileh->bfReserved2;
    *(unsigned int*)  (&out.raw()[10])  = fileh->bfOffBits;
    return out;
}
ZBinary writeInfoHeader(const BitmapInfoHeader *infoh){
    ZBinary out;
    out.fill(0, 40);
    *(unsigned int*)  (&out.raw()[0])   = infoh->biSize;
    *(unsigned int*)  (&out.raw()[4])   = infoh->biWidth;
    *(unsigned int*)  (&out.raw()[8])   = infoh->biHeight;
    *(unsigned short*)(&out.raw()[12])  = infoh->biPlanes;
    *(unsigned short*)(&out.raw()[14])  = infoh->biBitCount;
    *(unsigned int*)  (&out.raw()[16])  = infoh->biCompression;
    *(unsigned int*)  (&out.raw()[20])  = infoh->biSizeImage;
    *(unsigned int*)  (&out.raw()[24])  = infoh->biXPelsPerMeter;
    *(unsigned int*)  (&out.raw()[28])  = infoh->biYPelsPerMeter;
    *(unsigned int*)  (&out.raw()[32])  = infoh->biClrUsed;
    *(unsigned int*)  (&out.raw()[36])  = infoh->biClrImportant;
    return out;
}

bool ZBMP::read(ZPath path){
    try {
        ZBinary buffer = ZFile::readBinary(path);
        if(buffer.size() < 54){
            throw ZError("File too small", BMPError::badfile, false);
        }

        BitmapFileHeader fileh;
        readFileHeader(buffer.raw(), &fileh);

        if(fileh.bfType != 0x4d42){
            throw ZError("Not a BMP file", BMPError::notabmp, false);
        }
        if(fileh.bfSize != buffer.size()){
            throw ZError("Incorrect file size in file header", BMPError::incorrectsize, false);
        }

        BitmapInfoHeader infoh;
        readInfoHeader(buffer.raw() + 14, &infoh);

        if(infoh.biSize != 40){
            throw ZError("Unsupported info header length", BMPError::badinfoheader, false);
        }
        if(infoh.biCompression != BI_RGB){
            throw ZError(ZString("Unsupported compression: ") << infoh.biCompression, BMPError::badcompression, false);
        }
        if(infoh.biBitCount != 24){
            throw ZError(ZString("Unsupported pixel bit count: ") << infoh.biBitCount, BMPError::badbitcount, false);
        }

        zu64 width = infoh.biWidth;
        zu64 height = infoh.biHeight;

        image.setDimensions(width, height, bmp_channels, 8);

        unsigned char *pixels = convertBMPDatatoRGB(buffer.raw() + fileh.bfOffBits, image.width(), image.height());

        image.takeData(pixels);

    } catch(ZError e){
        error = e;
        //ELOG("BMP Read error " << e.code() << ": " << e.what());
        return false;
    }

    return true;
}

bool ZBMP::write(ZPath path){

    if(!image.isRGB24()){
        error = ZError(ZString("BMP Write: Invalid channels / depth ") + image.channels() + " " + image.depth());
        return false;
    }

    zu64 outsize;
    unsigned char *dataout = convertRGBtoBMPData(image.buffer(), image.width(), image.height(), outsize);
    if(!dataout){
        error = ZError("BMP Write: error in RGB conversion");
        return false;
    }

    // sizes of headers in file
    const zu8 fileheadersize = 14;
    const zu8 infoheadersize = 40;

    const zu32 filesize = fileheadersize + infoheadersize + outsize;

    ZBinary out;

    BitmapFileHeader fileh;
    memset(&fileh, 0, sizeof(BitmapFileHeader));
    fileh.bfType = 0x4d42;  // magic bmp signature
    fileh.bfSize = filesize;
    fileh.bfOffBits = fileheadersize + infoheadersize;
    out.concat(writeFileHeader(&fileh));

    BitmapInfoHeader infoh;
    memset(&infoh, 0, sizeof(BitmapInfoHeader));
    infoh.biSize = infoheadersize;
    infoh.biWidth = image.width();
    infoh.biHeight = image.height();
    infoh.biPlanes = 1;
    infoh.biBitCount = image.channels() * image.depth();    // will always be 24, but still
    infoh.biCompression = 0;    // no compression
    out.concat(writeInfoHeader(&infoh));

    out.concat(ZBinary(dataout, outsize));
    delete[] dataout;

    if(!ZFile::writeBinary(path, out)){
        error = ZError("BMP Write: bad write file");
        return false;
    }

    return true;
}

unsigned char *ZBMP::convertBMPDatatoRGB(unsigned char *bmpbuffer, zu32 height, zu32 width){
    if ( ( NULL == bmpbuffer ) || ( width == 0 ) || ( height == 0 ) )
        return NULL;

    zu32 padding = 0;
    zu32 scanlinebytes = width * 3;
    while((scanlinebytes + padding) % 4 != 0)
        padding++;
    zu32 psw = scanlinebytes + padding;

    unsigned char* newbuf = new unsigned char[width * height * 3];

    zu64 bufpos = 0;
    zu64 newpos = 0;
    for ( zu64 y = 0; y < height; y++ ){
        for ( zu64 x = 0; x < 3 * width; x+=3 ){
            newpos = y * 3 * width + x;
            bufpos = ( height - y - 1 ) * psw + x;

            newbuf[newpos]     = bmpbuffer[bufpos + 2];
            newbuf[newpos + 1] = bmpbuffer[bufpos + 1];
            newbuf[newpos + 2] = bmpbuffer[bufpos];
        }
    }
    return newbuf;
}

unsigned char *ZBMP::convertRGBtoBMPData(unsigned char *rgbbuffer, zu32 height, zu32 width, zu64 &outsize){
    if((rgbbuffer == NULL) || (width == 0) || (height == 0))
        return NULL;

    zu32 padding = 0;
    zu32 scanlinebytes = width * 3;
    while((scanlinebytes + padding) % 4 != 0)
        padding++;
    zu32 psw = scanlinebytes + padding;

    outsize = height * psw;
    unsigned char *newbuf = new unsigned char[outsize];

    memset(newbuf, 0, outsize);

    zu64 bufpos = 0;
    zu64 newpos = 0;
    for(zu32 y = 0; y < height; y++){
        for(zu32 x = 0; x < 3 * width; x += 3){
            bufpos = y * 3 * width + x;                 // position in original buffer
            newpos = (height - y - 1) * psw + x;        // position in padded buffer

            newbuf[newpos]     = rgbbuffer[bufpos + 2]; // swap r and b
            newbuf[newpos + 1] = rgbbuffer[bufpos + 1]; // g stays
            newbuf[newpos + 2] = rgbbuffer[bufpos];     // swap b and r
        }
    }
    return newbuf;

}

}
