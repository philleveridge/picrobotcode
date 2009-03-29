using System;
using System.Collections.Generic;
using System.Text;
using System.IO;

namespace KadeSoft
{
    class proccesSound
    {
        WaveFileReader reader;
        int channels = 1;

        public proccesSound(BinaryWriter outwave, WaveFileReader reader, WaveFile contents, string vol)
        {
            this.reader = reader;

            try
            {
                outwave.Write(contents.maindata.sGroupID.ToCharArray());
                outwave.Write(contents.maindata.dwFileLength - 8);
                outwave.Write(contents.maindata.sRiffType.ToCharArray());
                outwave.Write(contents.format.sChunkID.ToCharArray());
                outwave.Write(contents.format.dwChunkSize);
                outwave.Write(contents.format.wFormatTag);
                outwave.Write(contents.format.wChannels);
                outwave.Write(contents.format.dwSamplesPerSec);
                outwave.Write(contents.format.dwAvgBytesPerSec);
                outwave.Write(contents.format.wBlockAlign);
                outwave.Write(contents.format.dwBitsPerSample);
                outwave.Write(contents.fact.sChunkID.ToCharArray());
                outwave.Write(contents.fact.dwChunkSize);
                outwave.Write(contents.fact.dwNumSamples);
                outwave.Write(contents.data.sChunkID.ToCharArray());
                outwave.Write(contents.data.dwChunkSize);
                bool result;
                //if (contents.format.dwBitsPerSample == 16)
                //    result = reader.AdjustVolume16(outwave, volume);
                // else
                if (contents.format.dwBitsPerSample == 8)
                    result = Binwav(outwave);
                else
                    result = false;
                if (result)
                    Console.WriteLine("Operation successful!");
                else
                    Console.WriteLine("Operation failed.");
                outwave.Close();
                return;
            }
            catch (FormatException)
            {
                Console.WriteLine("Please enter a valid volume value.");
                return;
            }
            catch (Exception e)
            {
                Console.WriteLine(e);
                return;
            }

        }




}
