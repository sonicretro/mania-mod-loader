using IniFile;
using ModManagerCommon;
using System;
using System.Collections.Generic;
using System.IO;

namespace ManiaModManager
{
	public class ManiaModInfo : ModInfo
	{
		public bool BlueSpheresTempoChange { get; set; }
		public bool SpeedShoesTempoChange { get; set; }
		[IniCollection(IniCollectionMode.IndexOnly)]
		public Dictionary<string,int> MusicLoops { get; set; }

		public static new IEnumerable<string> GetModFiles(DirectoryInfo directoryInfo)
		{
			string modini = Path.Combine(directoryInfo.FullName, "mod.ini");
			if (File.Exists(modini))
			{
				yield return modini;
				yield break;
			}

			foreach (DirectoryInfo item in directoryInfo.GetDirectories())
			{
				if (item.Name.Equals("data", StringComparison.OrdinalIgnoreCase) || item.Name[0] == '.')
				{
					continue;
				}

				foreach (string filename in GetModFiles(item))
					yield return filename;
			}
		}
	}
}
