using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Security.Cryptography;
using System.Windows.Forms;
using IniFile;

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
		LoaderInfo loaderini;
		Dictionary<string, ModInfo> mods;
		bool installed;
		bool suppressEvent;

		private static void SetDoubleBuffered(Control control, bool enable)
		{
			PropertyInfo doubleBufferPropertyInfo = control.GetType().GetProperty("DoubleBuffered", BindingFlags.Instance | BindingFlags.NonPublic);
			doubleBufferPropertyInfo?.SetValue(control, enable, null);
		}

		private void MainForm_Load(object sender, EventArgs e)
		{
			SetDoubleBuffered(modListView, true);
			loaderini = File.Exists(loaderinipath) ? IniSerializer.Deserialize<LoaderInfo>(loaderinipath) : new LoaderInfo();

			LoadModList();

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

		private void LoadModList()
		{
			modListView.Items.Clear();
			mods = new Dictionary<string, ModInfo>();
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

			IniSerializer.Serialize(loaderini, loaderinipath);
		}

		private void saveAndPlayButton_Click(object sender, EventArgs e)
		{
			Save();
			Process process = Process.Start("SonicMania.exe");
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

		private void modListView_ItemCheck(object sender, ItemCheckEventArgs e)
		{
			if (suppressEvent) return;
			string modDir = Path.Combine(Environment.CurrentDirectory, "mods");
			List<string> modlist = new List<string>();
			foreach (ListViewItem item in modListView.CheckedItems)
				modlist.Add((string)item.Tag);
			if (e.NewValue == CheckState.Unchecked)
				modlist.Remove((string)modListView.Items[e.Index].Tag);
			else
				modlist.Add((string)modListView.Items[e.Index].Tag);
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
	}
}
