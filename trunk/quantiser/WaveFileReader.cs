using System;
using System.IO;
using System.Text;
using System.Collections;
using System.Collections.Specialized;


namespace KadeSoft
{
	/// <summary>
	/// This class gives you repurposable read/write access to a wave file.
	/// </summary>
	public class WaveFileReader : IDisposable
	{
		BinaryReader reader;

		riffChunk mainfile;
		fmtChunk format;
		factChunk fact;
		dataChunk data;

#region General Utilities
		/*
		 * WaveFileReader(string) - 2004 July 28
		 * A fairly standard constructor that opens a file using the filename supplied to it.
		 */
		public WaveFileReader(string filename)
		{
			reader = new BinaryReader(new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read));
		}

		/*
		 * long GetPosition() - 2004 July 28
		 * Returns the current position of the reader's BaseStream.
		 */
		public long GetPosition()
		{
			return reader.BaseStream.Position;
		}

		/*
		 * string GetChunkName() - 2004 July 29
		 * Reads the next four bytes from the file, converts the 
		 * char array into a string, and returns it.
		 */
		public string GetChunkName()
		{
			return new string(reader.ReadChars(4));
		}

		/*
		 * void AdvanceToNext() - 2004 August 2
		 * Advances to the next chunk in the file.  This is fine, 
		 * since we only really care about the fmt and data 
		 * streams for now.
		 */
		public void AdvanceToNext()
		{
			long NextOffset = (long) reader.ReadUInt32(); //Get next chunk offset
			//Seek to the next offset from current position
			reader.BaseStream.Seek(NextOffset,SeekOrigin.Current);
		}
#endregion
#region Header Extraction Methods
		/*
		 * WaveFileFormat ReadMainFileHeader - 2004 July 28
		 * Read in the main file header.  Not much more to say, really.
		 * For XML serialization purposes, I "correct" the dwFileLength
		 * field to describe the whole file's length.
		 */
		public riffChunk ReadMainFileHeader()
		{
			mainfile = new riffChunk();

			mainfile.sGroupID = new string(reader.ReadChars(4));
			mainfile.dwFileLength = reader.ReadUInt32()+8;
			mainfile.sRiffType = new string(reader.ReadChars(4));
			return mainfile;
		}

		//fmtChunk ReadFormatHeader() - 2004 July 28
		//Again, not much to say.
		public fmtChunk ReadFormatHeader()
		{
			format = new fmtChunk();

			format.sChunkID = "fmt ";
			format.dwChunkSize = reader.ReadUInt32();
			format.wFormatTag = reader.ReadUInt16();
			format.wChannels = reader.ReadUInt16();
			format.dwSamplesPerSec = reader.ReadUInt32();
			format.dwAvgBytesPerSec = reader.ReadUInt32();
			format.wBlockAlign = reader.ReadUInt16();
			format.dwBitsPerSample = reader.ReadUInt32();
			return format;
		} 

		//factChunk ReadFactHeader() - 2004 July 28
		//Again, not much to say.
		public factChunk ReadFactHeader()
		{
			fact = new factChunk();

			fact.sChunkID = "fact";
			fact.dwChunkSize = reader.ReadUInt32();
			fact.dwNumSamples = reader.ReadUInt32();
			return fact;
		} 


		//dataChunk ReadDataHeader() - 2004 July 28
		//Again, not much to say.
		public dataChunk ReadDataHeader()
		{
			data = new dataChunk();

			data.sChunkID = "data";
			data.dwChunkSize = reader.ReadUInt32();
			data.lFilePosition = reader.BaseStream.Position;
            //if (!fact.Equals(null))
            if (fact != null)
                data.dwNumSamples = fact.dwNumSamples;
			else
				data.dwNumSamples = data.dwChunkSize / (format.dwBitsPerSample/8 * format.wChannels);
			//The above could be written as data.dwChunkSize / format.wBlockAlign, but I want to emphasize what the frames look like.
			data.dwMinLength = (data.dwChunkSize / format.dwAvgBytesPerSec) / 60;
			data.dSecLength = ((double)data.dwChunkSize / (double)format.dwAvgBytesPerSec) - (double)data.dwMinLength*60;
			return data;
		}


        /*
         */
        public bool quantise(StreamWriter outfile, int level)
        {
            //Seek to the beginning of the data chunk
            int channels = 1; // might not be ?


            reader.BaseStream.Seek(data.lFilePosition, SeekOrigin.Begin);
            byte[] dataset1;
            int cs = 4444;

            int fmtn = 0;

            Console.WriteLine("const unsigned char sound[] = {");
            outfile.WriteLine("const unsigned char sound[] = {");
 
            try
            {
                for (ulong i = (ulong)(data.dwChunkSize / cs); i > 0; i--)
                {
                    dataset1 = reader.ReadBytes(cs);

                    int bp = 1;
                    int bo = 0;

                    for (ulong j = 0; j < (ulong)(cs); j++)
                    {
                        //Console.WriteLine("Data[" + i + "/"+ j + "]=" + dataset1[j]);

                        long t = 0;

                        for (ulong s = 0; s < 11; s++)
                        {
                            try
                            {
                                t += dataset1[j + s];
                            }
                            catch (Exception z)
                            {
                                Console.WriteLine("Err");
                            }
                        }

                        t = t / 11;

                        if (t > level)
                        {
                            //Console.WriteLine("1");
                            bo = bo | bp;
                        }
                        else
                        {
                            //Console.WriteLine("0");
                        }
                        bp = bp << 1;
                        if (bp == 256)
                        {
                            fmtn++;
                            if (fmtn == 1)
                            {
                                Console.Write("\t");
                                outfile.Write("\t");
                            }
                            Console.Write(bo + ",");
                            outfile.Write(bo + ",");
                            bp = 1;
                            bo = 0;
                            if (fmtn > 10)
                            {
                                Console.WriteLine("");
                                outfile.WriteLine("");
                                fmtn = 0;
                            }
                        }
                        j += 10;
                    }
                }

            }
            catch (Exception e)
            {
                Console.WriteLine("Error! " + e);
            }

            Console.WriteLine("\n};");
            outfile.Write("\n};");

            return true;
        }
#endregion
#region IDisposable Members

		public void Dispose() 
		{
			if(reader != null) 
				reader.Close();
		}

#endregion


	}
}
