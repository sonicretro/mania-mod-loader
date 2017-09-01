using System.Collections.Generic;
using IniFile;

namespace ManiaModManager
{
	class LoaderInfo
	{
		[IniName("Mod")]
		[IniCollection(IniCollectionMode.NoSquareBrackets, StartIndex = 1)]
		public List<string> Mods { get; set; }

		public LoaderInfo()
		{
			Mods = new List<string>();
		}
	}
}
