using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Windows.Forms;
using IniFile;
using ModManagerCommon;
using ModManagerCommon.Forms;
using Newtonsoft.Json;
using ValueType = ModManagerCommon.ValueType;

namespace ManiaModManager
{
	public partial class MainForm : Form
	{
		public MainForm()
		{
			InitializeComponent();
		}

		const string datadllpath = "d3d9.dll";
		const string loaderinipath = "mods/ManiaModLoader.ini";
		const string loaderdllpath = "mods/ManiaModLoader.dll";
		ManiaLoaderInfo loaderini;
		Dictionary<string, ModInfo> mods;
		const string codexmlpath = "mods/Codes.xml";
		const string codedatpath = "mods/Codes.dat";
		const string patchdatpath = "mods/Patches.dat";
		CodeList mainCodes;
		List<Code> codes;
		bool installed;
		bool suppressEvent;

		private static void SetDoubleBuffered(Control control, bool enable)
		{
			PropertyInfo doubleBufferPropertyInfo = control.GetType().GetProperty("DoubleBuffered", BindingFlags.Instance | BindingFlags.NonPublic);
			doubleBufferPropertyInfo?.SetValue(control, enable, null);
		}

		private void MainForm_Load(object sender, EventArgs e)
		{
			if (!Debugger.IsAttached)
				Environment.CurrentDirectory = Application.StartupPath;
			SetDoubleBuffered(modListView, true);
			loaderini = File.Exists(loaderinipath) ? IniSerializer.Deserialize<ManiaLoaderInfo>(loaderinipath) : new ManiaLoaderInfo();

			try { mainCodes = CodeList.Load(codexmlpath); }
			catch { mainCodes = new CodeList() { Codes = new List<Code>() }; }

			LoadModList();

			enableDebugConsole.Checked = loaderini.EnableConsole;
			startingScene.SelectedIndex = loaderini.StartingScene;
			speedShoesTempoCheckBox.Checked = loaderini.SpeedShoesTempoChange;
			blueSpheresTempoCheckBox.Checked = loaderini.BlueSpheresTempoChange;

			if (File.Exists(datadllpath))
			{
				installed = true;
				installButton.Text = "Uninstall loader";
				using (MD5 md5 = MD5.Create())
				{
					byte[] hash1 = md5.ComputeHash(File.ReadAllBytes(loaderdllpath));
					byte[] hash2 = md5.ComputeHash(File.ReadAllBytes(datadllpath));

					if (hash1.SequenceEqual(hash2))
					{
						return;
					}
				}

				DialogResult result = MessageBox.Show(this, "Installed loader DLL differs from copy in mods folder."
					+ "\n\nDo you want to overwrite the installed copy?", Text, MessageBoxButtons.YesNo, MessageBoxIcon.Warning);

				if (result == DialogResult.Yes)
					File.Copy(loaderdllpath, datadllpath, true);
			}
		}

		private void HandleUri(string uri)
		{
			if (WindowState == FormWindowState.Minimized)
			{
				WindowState = FormWindowState.Normal;
			}

			Activate();

			var fields = uri.Substring("smmm:".Length).Split(',');

			// TODO: lib-ify
			string itemType = fields.FirstOrDefault(x => x.StartsWith("gb_itemtype", StringComparison.InvariantCultureIgnoreCase));
			itemType = itemType.Substring(itemType.IndexOf(":") + 1);

			string itemId = fields.FirstOrDefault(x => x.StartsWith("gb_itemid", StringComparison.InvariantCultureIgnoreCase));
			itemId = itemId.Substring(itemId.IndexOf(":") + 1);

			var dummyInfo = new ModInfo();

			using (var client = new UpdaterWebClient())
			{
				var response = client.DownloadString(
					string.Format("https://api.gamebanana.com/Core/Item/Data?itemtype={0}&itemid={1}&fields=name,authors",
						itemType, itemId)
				);

				var array = JsonConvert.DeserializeObject<string[]>(response);
				dummyInfo.Name = array[0];

				var authors = JsonConvert.DeserializeObject<Dictionary<string, string[][]>>(array[1]);

				// for every array of string[] in authors, select the first element of each array
				var authorList = from i in (from x in authors select x.Value) from j in i select j[0];
				dummyInfo.Author = string.Join(", ", authorList);
			}

			DialogResult result = MessageBox.Show(this, $"Do you want to install mod \"{dummyInfo.Name}\" by {dummyInfo.Author} from {new Uri(fields[0]).DnsSafeHost}?", "Mod Download", MessageBoxButtons.YesNo, MessageBoxIcon.Question);

			if (result != DialogResult.Yes)
			{
				return;
			}

			string updatePath = Path.Combine("mods", ".updates");

			#region create update folder
			do
			{
				try
				{
					result = DialogResult.Cancel;
					if (!Directory.Exists(updatePath))
					{
						Directory.CreateDirectory(updatePath);
					}
				}
				catch (Exception ex)
				{
					result = MessageBox.Show(this, "Failed to create temporary update directory:\n" + ex.Message
					                               + "\n\nWould you like to retry?", "Directory Creation Failed", MessageBoxButtons.RetryCancel);
				}
			} while (result == DialogResult.Retry);
			#endregion

			string dummyPath = dummyInfo.Name;

			foreach (char c in Path.GetInvalidFileNameChars())
			{
				dummyPath = dummyPath.Replace(c, '_');
			}

			dummyPath = Path.Combine("mods", dummyPath);

			var updates = new List<ModDownload>
			{
				new ModDownload(dummyInfo, dummyPath, fields[0], null, 0)
			};

			using (var progress = new DownloadDialog(updates, updatePath))
			{
				progress.ShowDialog(this);
			}

			do
			{
				try
				{
					result = DialogResult.Cancel;
					Directory.Delete(updatePath, true);
				}
				catch (Exception ex)
				{
					result = MessageBox.Show(this, "Failed to remove temporary update directory:\n" + ex.Message
					                               + "\n\nWould you like to retry? You can remove the directory manually later.",
						"Directory Deletion Failed", MessageBoxButtons.RetryCancel);
				}
			} while (result == DialogResult.Retry);

			LoadModList();
		}

		private void MainForm_Shown(object sender, EventArgs e)
		{
			List<string> uris = Program.UriQueue.GetUris();

			foreach (string str in uris)
			{
				HandleUri(str);
			}

			Program.UriQueue.UriEnqueued += UriQueueOnUriEnqueued;
		}

		private void UriQueueOnUriEnqueued(object sender, OnUriEnqueuedArgs args)
		{
			args.Handled = true;

			if (InvokeRequired)
			{
				Invoke((Action<object, OnUriEnqueuedArgs>)UriQueueOnUriEnqueued, sender, args);
				return;
			}

			HandleUri(args.Uri);
		}

		private void LoadModList()
		{
			modListView.Items.Clear();
			mods = new Dictionary<string, ModInfo>();
			codes = new List<Code>(mainCodes.Codes);
			string modDir = Path.Combine(Environment.CurrentDirectory, "mods");

			foreach (string filename in ModInfo.GetModFiles(new DirectoryInfo(modDir)))
			{
				mods.Add(Path.GetDirectoryName(filename).Substring(modDir.Length + 1), IniSerializer.Deserialize<ModInfo>(filename));
			}

			modListView.BeginUpdate();

			foreach (string mod in new List<string>(loaderini.Mods))
			{
				if (mods.ContainsKey(mod))
				{
					ModInfo inf = mods[mod];
					suppressEvent = true;
					modListView.Items.Add(new ListViewItem(new[] { inf.Name, inf.Author, inf.Version }) { Checked = true, Tag = mod });
					suppressEvent = false;
					if (!string.IsNullOrEmpty(inf.Codes))
						codes.AddRange(CodeList.Load(Path.Combine(Path.Combine(modDir, mod), inf.Codes)).Codes);
				}
				else
				{
					MessageBox.Show(this, "Mod \"" + mod + "\" could not be found.\n\nThis mod will be removed from the list.",
						Text, MessageBoxButtons.OK, MessageBoxIcon.Warning);
					loaderini.Mods.Remove(mod);
				}
			}

			foreach (KeyValuePair<string, ModInfo> inf in mods)
			{
				if (!loaderini.Mods.Contains(inf.Key))
					modListView.Items.Add(new ListViewItem(new[] { inf.Value.Name, inf.Value.Author, inf.Value.Version }) { Tag = inf.Key });
			}

			modListView.EndUpdate();

			loaderini.EnabledCodes = new List<string>(loaderini.EnabledCodes.Where(a => codes.Any(c => c.Name == a)));
			foreach (Code item in codes.Where(a => a.Required && !loaderini.EnabledCodes.Contains(a.Name)))
				loaderini.EnabledCodes.Add(item.Name);

			codesCheckedListBox.BeginUpdate();
			codesCheckedListBox.Items.Clear();

			foreach (Code item in codes)
				codesCheckedListBox.Items.Add(item.Name, loaderini.EnabledCodes.Contains(item.Name));

			codesCheckedListBox.EndUpdate();
		}

		private void modListView_SelectedIndexChanged(object sender, EventArgs e)
		{
			int count = modListView.SelectedIndices.Count;
			if (count == 0)
			{
				modUpButton.Enabled = modDownButton.Enabled = false;
				modDescription.Text = "Description: No mod selected.";
			}
			else if (count == 1)
			{
				modDescription.Text = "Description: " + mods[(string)modListView.SelectedItems[0].Tag].Description;
				modUpButton.Enabled = modListView.SelectedIndices[0] > 0;
				modDownButton.Enabled = modListView.SelectedIndices[0] < modListView.Items.Count - 1;
			}
			else if (count > 1)
			{
				modDescription.Text = "Description: Multiple mods selected.";
				modUpButton.Enabled = modDownButton.Enabled = true;
			}
		}

		private void modUpButton_Click(object sender, EventArgs e)
		{
			if (modListView.SelectedItems.Count < 1)
				return;

			modListView.BeginUpdate();

			for (int i = 0; i < modListView.SelectedItems.Count; i++)
			{
				int index = modListView.SelectedItems[i].Index;

				if (index-- > 0 && !modListView.Items[index].Selected)
				{
					ListViewItem item = modListView.SelectedItems[i];
					modListView.Items.Remove(item);
					modListView.Items.Insert(index, item);
				}
			}

			modListView.SelectedItems[0].EnsureVisible();
			modListView.EndUpdate();
		}

		private void modDownButton_Click(object sender, EventArgs e)
		{
			if (modListView.SelectedItems.Count < 1)
				return;

			modListView.BeginUpdate();

			for (int i = modListView.SelectedItems.Count - 1; i >= 0; i--)
			{
				int index = modListView.SelectedItems[i].Index + 1;

				if (index != modListView.Items.Count && !modListView.Items[index].Selected)
				{
					ListViewItem item = modListView.SelectedItems[i];
					modListView.Items.Remove(item);
					modListView.Items.Insert(index, item);
				}
			}

			modListView.SelectedItems[modListView.SelectedItems.Count - 1].EnsureVisible();
			modListView.EndUpdate();
		}

		private void Save()
		{
			loaderini.Mods.Clear();

			foreach (ListViewItem item in modListView.CheckedItems)
			{
				loaderini.Mods.Add((string)item.Tag);
			}

			loaderini.EnableConsole = enableDebugConsole.Checked;
			loaderini.StartingScene = startingScene.SelectedIndex;
			loaderini.SpeedShoesTempoChange = speedShoesTempoCheckBox.Checked;
			loaderini.BlueSpheresTempoChange = blueSpheresTempoCheckBox.Checked;

			IniSerializer.Serialize(loaderini, loaderinipath);

			List<Code> selectedCodes = new List<Code>();
			List<Code> selectedPatches = new List<Code>();

			foreach (Code item in codesCheckedListBox.CheckedIndices.OfType<int>().Select(a => codes[a]))
			{
				if (item.Patch)
					selectedPatches.Add(item);
				else
					selectedCodes.Add(item);
			}

			using (FileStream fs = File.Create(patchdatpath))
			using (BinaryWriter bw = new BinaryWriter(fs, System.Text.Encoding.ASCII))
			{
				bw.Write(new[] { 'c', 'o', 'd', 'e', 'v', '5' });
				bw.Write(selectedPatches.Count);
				foreach (Code item in selectedPatches)
				{
					if (item.IsReg)
						bw.Write((byte)CodeType.newregs);
					WriteCodes(item.Lines, bw);
				}
				bw.Write(byte.MaxValue);
			}
			using (FileStream fs = File.Create(codedatpath))
			using (BinaryWriter bw = new BinaryWriter(fs, System.Text.Encoding.ASCII))
			{
				bw.Write(new[] { 'c', 'o', 'd', 'e', 'v', '5' });
				bw.Write(selectedCodes.Count);
				foreach (Code item in selectedCodes)
				{
					if (item.IsReg)
						bw.Write((byte)CodeType.newregs);
					WriteCodes(item.Lines, bw);
				}
				bw.Write(byte.MaxValue);
			}
		}

		private void WriteCodes(IEnumerable<CodeLine> codeList, BinaryWriter writer)
		{
			foreach (CodeLine line in codeList)
			{
				writer.Write((byte)line.Type);
				uint address;
				if (line.Address.StartsWith("r"))
					address = uint.Parse(line.Address.Substring(1), System.Globalization.NumberStyles.None, System.Globalization.NumberFormatInfo.InvariantInfo);
				else
					address = uint.Parse(line.Address, System.Globalization.NumberStyles.HexNumber);
				if (line.Pointer)
					address |= 0x80000000u;
				writer.Write(address);
				if (line.Pointer)
					if (line.Offsets != null)
					{
						writer.Write((byte)line.Offsets.Count);
						foreach (int off in line.Offsets)
							writer.Write(off);
					}
					else
						writer.Write((byte)0);
				if (line.Type == CodeType.ifkbkey)
					writer.Write((int)(Keys)Enum.Parse(typeof(Keys), line.Value));
				else
					switch (line.ValueType)
					{
						case ValueType.@decimal:
							switch (line.Type)
							{
								case CodeType.writefloat:
								case CodeType.addfloat:
								case CodeType.subfloat:
								case CodeType.mulfloat:
								case CodeType.divfloat:
								case CodeType.ifeqfloat:
								case CodeType.ifnefloat:
								case CodeType.ifltfloat:
								case CodeType.iflteqfloat:
								case CodeType.ifgtfloat:
								case CodeType.ifgteqfloat:
									writer.Write(float.Parse(line.Value, System.Globalization.NumberStyles.Float, System.Globalization.NumberFormatInfo.InvariantInfo));
									break;
								default:
									writer.Write(unchecked((int)long.Parse(line.Value, System.Globalization.NumberStyles.Integer, System.Globalization.NumberFormatInfo.InvariantInfo)));
									break;
							}
							break;
						case ValueType.hex:
							writer.Write(uint.Parse(line.Value, System.Globalization.NumberStyles.HexNumber, System.Globalization.NumberFormatInfo.InvariantInfo));
							break;
					}
				writer.Write(line.RepeatCount ?? 1);
				if (line.IsIf)
				{
					WriteCodes(line.TrueLines, writer);
					if (line.FalseLines.Count > 0)
					{
						writer.Write((byte)CodeType.@else);
						WriteCodes(line.FalseLines, writer);
					}
					writer.Write((byte)CodeType.endif);
				}
			}
		}

		static readonly string[] scenelist =
		{
			"",
			"stage=Title;scene=1;",
			"stage=Menu;scene=1;",
			"stage=Thanks;scene=1;",
			"stage=LSelect;scene=1;",
			"stage=Credits;scene=1;",
			"stage=Ending;scene=C;",
			"stage=SPZ1;scene=1d;",
			"stage=GHZ;scene=1;",
			"stage=GHZ;scene=2;",
			"stage=CPZ;scene=1;",
			"stage=CPZ;scene=2;",
			"stage=SPZ1;scene=1;",
			"stage=SPZ2;scene=1;",
			"stage=FBZ;scene=1;",
			"stage=FBZ;scene=2;",
			"stage=PSZ1;scene=1;",
			"stage=PSZ2;scene=2;",
			"stage=SSZ1;scene=1;",
			"stage=SSZ2;scene=1;",
			"stage=SSZ2;scene=2;",
			"stage=HCZ;scene=1;",
			"stage=HCZ;scene=2;",
			"stage=MSZ;scene=1;",
			"stage=MSZ;scene=1k;",
			"stage=MSZ;scene=2;",
			"stage=OOZ1;scene=1;",
			"stage=OOZ2;scene=2;",
			"stage=LRZ1;scene=1;",
			"stage=LRZ2;scene=1;",
			"stage=LRZ3;scene=1;",
			"stage=MMZ;scene=1;",
			"stage=MMZ;scene=2;",
			"stage=TMZ1;scene=1;",
			"stage=TMZ2;scene=1;",
			"stage=TMZ3;scene=1;",
			"stage=ERZ;scene=1;",
			"stage=UFO1;scene=1;",
			"stage=UFO2;scene=1;",
			"stage=UFO3;scene=1;",
			"stage=UFO4;scene=1;",
			"stage=UFO5;scene=1;",
			"stage=UFO6;scene=1;",
			"stage=UFO7;scene=1;",
			"stage=SpecialBS;scene=1;",
			"stage=SpecialBS;scene=2;",
			"stage=SpecialBS;scene=3;",
			"stage=SpecialBS;scene=4;",
			"stage=SpecialBS;scene=5;",
			"stage=SpecialBS;scene=6;",
			"stage=SpecialBS;scene=7;",
			"stage=SpecialBS;scene=8;",
			"stage=SpecialBS;scene=9;",
			"stage=SpecialBS;scene=10;",
			"stage=SpecialBS;scene=11;",
			"stage=SpecialBS;scene=12;",
			"stage=SpecialBS;scene=13;",
			"stage=SpecialBS;scene=14;",
			"stage=SpecialBS;scene=15;",
			"stage=SpecialBS;scene=16;",
			"stage=SpecialBS;scene=17;",
			"stage=SpecialBS;scene=18;",
			"stage=SpecialBS;scene=19;",
			"stage=SpecialBS;scene=20;",
			"stage=SpecialBS;scene=21;",
			"stage=SpecialBS;scene=22;",
			"stage=SpecialBS;scene=23;",
			"stage=SpecialBS;scene=24;",
			"stage=SpecialBS;scene=25;",
			"stage=SpecialBS;scene=26;",
			"stage=SpecialBS;scene=27;",
			"stage=SpecialBS;scene=28;",
			"stage=SpecialBS;scene=29;",
			"stage=SpecialBS;scene=30;",
			"stage=SpecialBS;scene=31;",
			"stage=SpecialBS;scene=32;",
			"stage=SpecialBS;scene=34;",
			"stage=SpecialBS;scene=36;",
			"stage=Puyo;scene=1;",
			"stage=DAGarden;scene=1;",
			"stage=AIZ;scene=1;",
			"stage=GHZCutscene;scene=1;",
			"stage=GHZCutscene;scene=2;",
			"stage=MSZCutscene;scene=1;",
			"stage=TimeTravel;scene=1;",
			"stage=Ending;scene=T;",
			"stage=Ending;scene=BS;",
			"stage=Ending;scene=BT;",
			"stage=Ending;scene=BK;",
			"stage=Ending;scene=G;",
			"stage=Ending;scene=TK;"
		};

		private void saveAndPlayButton_Click(object sender, EventArgs e)
		{
			Save();
			System.Text.StringBuilder sb = new System.Text.StringBuilder();
			if (enableDebugConsole.Checked)
				sb.Append("console=true;");
			if (startingScene.SelectedIndex > 0)
				sb.Append(scenelist[startingScene.SelectedIndex]);
			Process process = Process.Start("SonicMania.exe", sb.ToString());
			process?.WaitForInputIdle(10000);
			Close();
		}

		private void saveButton_Click(object sender, EventArgs e)
		{
			Save();
			LoadModList();
		}

		private void installButton_Click(object sender, EventArgs e)
		{
			if (installed)
			{
				File.Delete(datadllpath);
				installButton.Text = "Install loader";
			}
			else
			{
				File.Copy(loaderdllpath, datadllpath);
				installButton.Text = "Uninstall loader";
			}
			installed = !installed;
		}

		private void buttonRefreshModList_Click(object sender, EventArgs e)
		{
			LoadModList();
		}

		private void codesCheckedListBox_ItemCheck(object sender, ItemCheckEventArgs e)
		{
			Code code = codes[e.Index];
			if (code.Required)
				e.NewValue = CheckState.Checked;
			if (e.NewValue == CheckState.Unchecked)
			{
				if (loaderini.EnabledCodes.Contains(code.Name))
					loaderini.EnabledCodes.Remove(code.Name);
			}
			else
			{
				if (!loaderini.EnabledCodes.Contains(code.Name))
					loaderini.EnabledCodes.Add(code.Name);
			}
		}

		private void modListView_ItemCheck(object sender, ItemCheckEventArgs e)
		{
			if (suppressEvent) return;
			codes = new List<Code>(mainCodes.Codes);
			string modDir = Path.Combine(Environment.CurrentDirectory, "mods");
			List<string> modlist = new List<string>();
			foreach (ListViewItem item in modListView.CheckedItems)
				modlist.Add((string)item.Tag);
			if (e.NewValue == CheckState.Unchecked)
				modlist.Remove((string)modListView.Items[e.Index].Tag);
			else
				modlist.Add((string)modListView.Items[e.Index].Tag);
			foreach (string mod in modlist)
				if (mods.ContainsKey(mod))
				{
					ModInfo inf = mods[mod];
					if (!string.IsNullOrEmpty(inf.Codes))
						codes.AddRange(CodeList.Load(Path.Combine(Path.Combine(modDir, mod), inf.Codes)).Codes);
				}
			loaderini.EnabledCodes = new List<string>(loaderini.EnabledCodes.Where(a => codes.Any(c => c.Name == a)));
			foreach (Code item in codes.Where(a => a.Required && !loaderini.EnabledCodes.Contains(a.Name)))
				loaderini.EnabledCodes.Add(item.Name);
			codesCheckedListBox.BeginUpdate();
			codesCheckedListBox.Items.Clear();
			foreach (Code item in codes)
				codesCheckedListBox.Items.Add(item.Name, loaderini.EnabledCodes.Contains(item.Name));
			codesCheckedListBox.EndUpdate();
		}

		private void modListView_MouseClick(object sender, MouseEventArgs e)
		{
			if (e.Button != MouseButtons.Right)
			{
				return;
			}

			if (modListView.FocusedItem.Bounds.Contains(e.Location))
			{
				modContextMenu.Show(Cursor.Position);
			}
		}

		private void openFolderToolStripMenuItem_Click(object sender, EventArgs e)
		{
			foreach (ListViewItem item in modListView.SelectedItems)
			{
				Process.Start(Path.Combine("mods", (string)item.Tag));
			}
		}

		private void installURLHandlerButton_Click(object sender, EventArgs e)
		{
			Process.Start(new ProcessStartInfo(Application.ExecutablePath, "urlhandler") { UseShellExecute = true, Verb = "runas" }).WaitForExit();
			MessageBox.Show(this, "URL handler installed!", Text);
		}
	}
}
