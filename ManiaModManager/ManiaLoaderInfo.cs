using ModManagerCommon;

namespace ManiaModManager
{
	class ManiaLoaderInfo : LoaderInfo
	{
		public bool EnableConsole { get; set; }
		public int StartingScene { get; set; }
		public int Platform { get; set; }
		public int Region { get; set; }
		public bool UseOriginalMusicPlayer { get; set; }
		public bool SpeedShoesTempoChange { get; set; }
		public bool BlueSpheresTempoChange { get; set; }
	}
}
