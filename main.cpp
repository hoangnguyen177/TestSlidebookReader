#include <QCoreApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QUuid>
#include <QByteArray>
#include <QBuffer>
#include <QByteArray>
#include <QDataStream>
#include "SBReadFile.h"
#include <QThread>
#include <iostream>
#include <string>
#include <cstdlib>
#include <cstdio>
#include <csignal>
#include <cstring>

#include "tiff.h"
#include "tiffio.h"

class tiff_exception : public std::exception
{
    using std::exception::exception;
};

/**
 * @brief
 * This small test project reads a slidebook
 * and exports to tiff files based on timepoints
 * @param argc
 * @param argv
 * @return
 *
 */
using TIFF = struct tiff;

void tiff_write_page_contig(TIFF *tiff, size_t w, size_t h, size_t num_channels,
                                 size_t page, size_t maxPage, uint16_t *data)
{
    TIFFSetField(tiff, TIFFTAG_IMAGEWIDTH, static_cast<uint32_t>(w));
    TIFFSetField(tiff, TIFFTAG_IMAGELENGTH, static_cast<uint32_t>(h));
    TIFFSetField(tiff, TIFFTAG_COMPRESSION, COMPRESSION_NONE);
    TIFFSetField(tiff, TIFFTAG_SUBFILETYPE, FILETYPE_PAGE);
    TIFFSetField(tiff, TIFFTAG_PAGENUMBER, static_cast<uint16_t>(page), static_cast<uint16_t>(maxPage));
    TIFFSetField(tiff, TIFFTAG_RESOLUTIONUNIT, static_cast<uint16_t>(1));

    TIFFSetField(tiff, TIFFTAG_PLANARCONFIG, static_cast<uint16_t>(PLANARCONFIG_CONTIG));


    TIFFSetField(tiff, TIFFTAG_SAMPLESPERPIXEL, static_cast<uint16_t>(num_channels));
    for(size_t i = 0; i < num_channels; ++i)
    {
        TIFFSetField(tiff, TIFFTAG_SAMPLEFORMAT, static_cast<uint16_t>(SAMPLEFORMAT_UINT));
        TIFFSetField(tiff, TIFFTAG_BITSPERSAMPLE, static_cast<uint32_t>(16));
    }

    TIFFSetField(tiff, TIFFTAG_PHOTOMETRIC, static_cast<uint16_t>(PHOTOMETRIC_RGB));

    /* Extra samples */
    if(num_channels > 3)
    {
        uint16_t extra[2] = {EXTRASAMPLE_UNSPECIFIED, EXTRASAMPLE_UNSPECIFIED};
        TIFFSetField(tiff, TIFFTAG_EXTRASAMPLES, num_channels - 3, extra);
    }

    for(size_t j = 0; j < h; ++j)
    {
        if(TIFFWriteScanline(tiff, data + (w * j) * num_channels, j, 0) < 0)
            throw tiff_exception();
    }

    if(!TIFFWriteDirectory(tiff))
        throw tiff_exception();
}


int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QCoreApplication::setApplicationName("TestSlidebookKafka");
    QCoreApplication::setApplicationVersion("1.0");
    QCommandLineParser parser;
    parser.setApplicationDescription("Test helper");
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument("input", QCoreApplication::translate("main", "Slidebook file."));
    parser.process(a);
    const QStringList args = parser.positionalArguments();


    QString file= QString("/home/hoangnguyen177/Desktop/working/Streaming/slidebook_files/MB231_gfp-Lifeact_01.sld");
    try{
        III::SBReadFile* sb_read_file = III_NewSBReadFile(file.toStdString().c_str(), III::kNoExceptionsMasked);
        CaptureIndex number_captures = sb_read_file->GetNumCaptures();
        for (int capture_index = 0; capture_index < number_captures; capture_index++)
        {
            //qDebug() << "Capture index:" << capture_index;
            PositionIndex number_channels = sb_read_file->GetNumChannels(capture_index);
            ChannelIndex number_positions = sb_read_file->GetNumPositions(capture_index);
            TimepointIndex number_timepoints = sb_read_file->GetNumTimepoints(capture_index);

            SInt32 xDim = sb_read_file->GetNumXColumns(capture_index);
            SInt32 yDim = sb_read_file->GetNumYRows(capture_index);
            SInt32 zDim = sb_read_file->GetNumZPlanes(capture_index);

            bool has_voxel_size = false;
            float voxel_size[3];
            has_voxel_size = sb_read_file->GetVoxelSize(capture_index, voxel_size[0], voxel_size[1], voxel_size[2]);
            if (!has_voxel_size)
            {
                voxel_size[0] = voxel_size[1] = voxel_size[2] = 1.0;
            }

            std::size_t planeSize = xDim * yDim;
            int bufferSize = xDim * yDim * zDim;
            int bufferSizeInBytes = bufferSize*sizeof(UInt16);
            UInt16* buffer = new UInt16[bufferSize];

            UInt16* planeBuffer = new UInt16[planeSize];
            for (int timepoint_index = 0;  timepoint_index < number_timepoints; timepoint_index++)
            {
                //qDebug() << "Time point:" << timepoint_index;
                for (int channel = 0; channel < number_channels; channel++)
                {
                    //qDebug() << "Channel:" << channel;
                    QString imagePath = QStringLiteral("/home/hoangnguyen177/Desktop/working/Streaming/Test/TestSlidebookReader/cap%1-ch%2-t%3.tif")
                            .arg(capture_index)
                            .arg(channel)
                            .arg(timepoint_index);
                    TIFF* tiff = TIFFOpen(imagePath.toStdString().c_str(), "w");
                    for (int z = 0; z < zDim; z++)
                    {
                        //qDebug() << "z:" << z;
                        bool success = sb_read_file->ReadImagePlaneBuf(planeBuffer,
                                                        capture_index, 0, timepoint_index, z, channel);
                        //qDebug() << "Successfully read:" << success;
                        tiff_write_page_contig(tiff, xDim, yDim, 1, z, zDim, planeBuffer);
                    }
                    TIFFClose(tiff);
                }
                QThread::sleep(1);
            }
            delete[] planeBuffer;
            delete[] buffer;
        }
    }
    catch (const III::Exception * e)
    {
        qDebug() << "Failed with exception:" << e->GetDescription();
        delete e;
        exit(1);
    }


    return a.exec();
}

