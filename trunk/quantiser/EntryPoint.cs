using System;
using System.IO;
using System.Runtime.InteropServices;

using System.Xml.Serialization;

namespace KadeSoft
{
 class EntryPoint
  {
	[STAThread]
	/*
	 * void Main(string[]) - 2004 August 2
	 * The entry point method for the assembly.  This calls all other functions, directly or indirectly.
	 */
	static void Main(string[] args)
	{ 
        args = new string[2];

        args[0] = "test.wav";
        args[1] = "test.xml";

		//We'll use the XmlSerializer class, eventually, to create an XML filedump of 
		//the wave file information.
		XmlSerializer xmlout = new XmlSerializer(typeof(WaveFile));
		Stream writer = new FileStream(args[1], FileMode.Create);
		string temp;

		//We use a custom filereader called WaveFileReader to retrieve the data from the
		//wave files.  In addition to conforming to good coding conventions, this streamlines
		//the code: here we just look at the "big picture", while in the WaveFileReader class
		//we only care what's going on in one small place at a time.
		WaveFileReader reader = new WaveFileReader(args[0]);
		WaveFile contents = new WaveFile();
		contents.maindata = reader.ReadMainFileHeader(); 
		contents.maindata.FileName = args[0];
		while (reader.GetPosition() < (long) contents.maindata.dwFileLength)
		{
			temp = reader.GetChunkName();
			if (temp=="fmt ")
			{
				contents.format = reader.ReadFormatHeader();
				if (reader.GetPosition()+contents.format.dwChunkSize == contents.maindata.dwFileLength)
					break;
			}
			else if (temp=="fact")
			{
				contents.fact = reader.ReadFactHeader();
				if (reader.GetPosition()+contents.fact.dwChunkSize == contents.maindata.dwFileLength)
					break;
			}
			else if (temp=="data")
			{
				contents.data = reader.ReadDataHeader();
				if (reader.GetPosition()+contents.data.dwChunkSize == contents.maindata.dwFileLength)
					break;

                //
                Console.WriteLine("test-" + contents.data.dSecLength);
                StreamWriter f = new StreamWriter("test.bin");
                reader.quantise(f, 128);
                f.Close();
			}
			else
			{	//This provides the required skipping of unsupported chunks.
				reader.AdvanceToNext();
			}
		}

		xmlout.Serialize(writer, contents);
		return;
	    }
  }

  public class WaveFile
  {
		public riffChunk maindata;
		public fmtChunk format;
		public factChunk fact;
		public dataChunk data;
  }
}
