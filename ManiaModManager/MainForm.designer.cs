namespace ManiaModManager
{
    partial class MainForm
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
			this.components = new System.ComponentModel.Container();
			System.Windows.Forms.Label label1;
			this.buttonRefreshModList = new System.Windows.Forms.Button();
			this.modDescription = new System.Windows.Forms.Label();
			this.modListView = new System.Windows.Forms.ListView();
			this.columnHeader1 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader2 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.columnHeader3 = ((System.Windows.Forms.ColumnHeader)(new System.Windows.Forms.ColumnHeader()));
			this.saveButton = new System.Windows.Forms.Button();
			this.saveAndPlayButton = new System.Windows.Forms.Button();
			this.installButton = new System.Windows.Forms.Button();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.modDownButton = new System.Windows.Forms.Button();
			this.modUpButton = new System.Windows.Forms.Button();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.codesCheckedListBox = new System.Windows.Forms.CheckedListBox();
			this.tabPage4 = new System.Windows.Forms.TabPage();
			this.startingScene = new System.Windows.Forms.ComboBox();
			this.enableDebugConsole = new System.Windows.Forms.CheckBox();
			this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
			this.modContextMenu = new System.Windows.Forms.ContextMenuStrip(this.components);
			this.openFolderToolStripMenuItem = new System.Windows.Forms.ToolStripMenuItem();
			label1 = new System.Windows.Forms.Label();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.tabPage4.SuspendLayout();
			this.modContextMenu.SuspendLayout();
			this.SuspendLayout();
			// 
			// label1
			// 
			label1.AutoSize = true;
			label1.Location = new System.Drawing.Point(6, 33);
			label1.Name = "label1";
			label1.Size = new System.Drawing.Size(80, 13);
			label1.TabIndex = 1;
			label1.Text = "Starting Scene:";
			// 
			// buttonRefreshModList
			// 
			this.buttonRefreshModList.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.buttonRefreshModList.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonRefreshModList.Location = new System.Drawing.Point(6, 223);
			this.buttonRefreshModList.Name = "buttonRefreshModList";
			this.buttonRefreshModList.Size = new System.Drawing.Size(75, 23);
			this.buttonRefreshModList.TabIndex = 3;
			this.buttonRefreshModList.Text = "&Refresh";
			this.buttonRefreshModList.UseVisualStyleBackColor = true;
			this.buttonRefreshModList.Click += new System.EventHandler(this.buttonRefreshModList_Click);
			// 
			// modDescription
			// 
			this.modDescription.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.modDescription.Location = new System.Drawing.Point(6, 252);
			this.modDescription.Name = "modDescription";
			this.modDescription.Size = new System.Drawing.Size(366, 36);
			this.modDescription.TabIndex = 6;
			this.modDescription.Text = "Description: No mod selected.";
			// 
			// modListView
			// 
			this.modListView.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.modListView.CheckBoxes = true;
			this.modListView.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
            this.columnHeader1,
            this.columnHeader2,
            this.columnHeader3});
			this.modListView.FullRowSelect = true;
			this.modListView.HideSelection = false;
			this.modListView.Location = new System.Drawing.Point(6, 6);
			this.modListView.Name = "modListView";
			this.modListView.Size = new System.Drawing.Size(337, 211);
			this.modListView.TabIndex = 0;
			this.modListView.UseCompatibleStateImageBehavior = false;
			this.modListView.View = System.Windows.Forms.View.Details;
			this.modListView.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.modListView_ItemCheck);
			this.modListView.SelectedIndexChanged += new System.EventHandler(this.modListView_SelectedIndexChanged);
			this.modListView.MouseClick += new System.Windows.Forms.MouseEventHandler(this.modListView_MouseClick);
			// 
			// columnHeader1
			// 
			this.columnHeader1.Text = "Name";
			this.columnHeader1.Width = 154;
			// 
			// columnHeader2
			// 
			this.columnHeader2.Text = "Author";
			this.columnHeader2.Width = 113;
			// 
			// columnHeader3
			// 
			this.columnHeader3.Text = "Version";
			// 
			// saveButton
			// 
			this.saveButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.saveButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.saveButton.Location = new System.Drawing.Point(87, 370);
			this.saveButton.Name = "saveButton";
			this.saveButton.Size = new System.Drawing.Size(75, 22);
			this.saveButton.TabIndex = 102;
			this.saveButton.Text = "&Save";
			this.saveButton.UseVisualStyleBackColor = true;
			this.saveButton.Click += new System.EventHandler(this.saveButton_Click);
			// 
			// saveAndPlayButton
			// 
			this.saveAndPlayButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.saveAndPlayButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.saveAndPlayButton.Location = new System.Drawing.Point(3, 370);
			this.saveAndPlayButton.Name = "saveAndPlayButton";
			this.saveAndPlayButton.Size = new System.Drawing.Size(78, 22);
			this.saveAndPlayButton.TabIndex = 101;
			this.saveAndPlayButton.Text = "Save && &Play";
			this.saveAndPlayButton.UseVisualStyleBackColor = true;
			this.saveAndPlayButton.Click += new System.EventHandler(this.saveAndPlayButton_Click);
			// 
			// installButton
			// 
			this.installButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left)));
			this.installButton.AutoSize = true;
			this.installButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.installButton.Location = new System.Drawing.Point(168, 370);
			this.installButton.Name = "installButton";
			this.installButton.Size = new System.Drawing.Size(80, 22);
			this.installButton.TabIndex = 103;
			this.installButton.Text = "Install loader";
			this.installButton.UseVisualStyleBackColor = true;
			this.installButton.Click += new System.EventHandler(this.installButton_Click);
			// 
			// tabControl1
			// 
			this.tabControl1.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
            | System.Windows.Forms.AnchorStyles.Left) 
            | System.Windows.Forms.AnchorStyles.Right)));
			this.tabControl1.Controls.Add(this.tabPage1);
			this.tabControl1.Controls.Add(this.tabPage2);
			this.tabControl1.Controls.Add(this.tabPage4);
			this.tabControl1.Location = new System.Drawing.Point(0, 0);
			this.tabControl1.Margin = new System.Windows.Forms.Padding(0, 0, 0, 3);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(386, 364);
			this.tabControl1.TabIndex = 0;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.Add(this.buttonRefreshModList);
			this.tabPage1.Controls.Add(this.modDescription);
			this.tabPage1.Controls.Add(this.modDownButton);
			this.tabPage1.Controls.Add(this.modUpButton);
			this.tabPage1.Controls.Add(this.modListView);
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Padding = new System.Windows.Forms.Padding(3);
			this.tabPage1.Size = new System.Drawing.Size(378, 338);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Mods";
			this.tabPage1.UseVisualStyleBackColor = true;
			// 
			// modDownButton
			// 
			this.modDownButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.modDownButton.AutoSize = true;
			this.modDownButton.Enabled = false;
			this.modDownButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.modDownButton.Location = new System.Drawing.Point(345, 35);
			this.modDownButton.Name = "modDownButton";
			this.modDownButton.Size = new System.Drawing.Size(27, 22);
			this.modDownButton.TabIndex = 2;
			this.modDownButton.Text = "↓";
			this.modDownButton.UseVisualStyleBackColor = true;
			this.modDownButton.Click += new System.EventHandler(this.modDownButton_Click);
			// 
			// modUpButton
			// 
			this.modUpButton.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Right)));
			this.modUpButton.AutoSize = true;
			this.modUpButton.Enabled = false;
			this.modUpButton.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.modUpButton.Location = new System.Drawing.Point(345, 6);
			this.modUpButton.Name = "modUpButton";
			this.modUpButton.Size = new System.Drawing.Size(27, 22);
			this.modUpButton.TabIndex = 1;
			this.modUpButton.Text = "↑";
			this.modUpButton.UseVisualStyleBackColor = true;
			this.modUpButton.Click += new System.EventHandler(this.modUpButton_Click);
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.Add(this.codesCheckedListBox);
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(378, 338);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Codes";
			this.tabPage2.UseVisualStyleBackColor = true;
			// 
			// codesCheckedListBox
			// 
			this.codesCheckedListBox.Dock = System.Windows.Forms.DockStyle.Fill;
			this.codesCheckedListBox.FormattingEnabled = true;
			this.codesCheckedListBox.Location = new System.Drawing.Point(0, 0);
			this.codesCheckedListBox.Name = "codesCheckedListBox";
			this.codesCheckedListBox.Size = new System.Drawing.Size(378, 338);
			this.codesCheckedListBox.TabIndex = 0;
			this.codesCheckedListBox.ItemCheck += new System.Windows.Forms.ItemCheckEventHandler(this.codesCheckedListBox_ItemCheck);
			// 
			// tabPage4
			// 
			this.tabPage4.Controls.Add(this.startingScene);
			this.tabPage4.Controls.Add(label1);
			this.tabPage4.Controls.Add(this.enableDebugConsole);
			this.tabPage4.Location = new System.Drawing.Point(4, 22);
			this.tabPage4.Name = "tabPage4";
			this.tabPage4.Padding = new System.Windows.Forms.Padding(3);
			this.tabPage4.Size = new System.Drawing.Size(378, 338);
			this.tabPage4.TabIndex = 3;
			this.tabPage4.Text = "Options";
			this.tabPage4.UseVisualStyleBackColor = true;
			// 
			// startingScene
			// 
			this.startingScene.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.startingScene.FormattingEnabled = true;
			this.startingScene.Items.AddRange(new object[] {
            "Logos (Default)",
            "Title Screen",
            "Menu",
            "Thanks For Playing",
            "Level Select",
            "Credits",
            "Continue",
            "Studiopolis Zone Act 1 Beta",
            "Green Hill Zone Act 1",
            "Green Hill Zone Act 2",
            "Chemical Plant Zone Act 1",
            "Chemical Plant Zone Act 2",
            "Studiopolis Zone Act 1",
            "Studiopolis Zone Act 2",
            "Flying Battery Zone Act 1",
            "Flying Battery Zone Act 2",
            "Press Garden Zone Act 1",
            "Press Garden Zone Act 2",
            "Stardust Speedway Zone Act 1",
            "Stardust Speedway Zone Act 2",
            "Stardust Speedway Zone Boss",
            "Hydrocity Zone Act 1",
            "Hydrocity Zone Act 2",
            "Mirage Saloon Zone Act 1 (Sonic+Tails)",
            "Mirage Saloon Zone Act 1 (Knuckles)",
            "Mirage Saloon Zone Act 2",
            "Oil Ocean Zone Act 1",
            "Oil Ocean Zone Act 2",
            "Lava Reef Zone Act 1",
            "Lava Reef Zone Act 2",
            "Lava Reef Zone Act 3",
            "Metallic Madness Zone Act 1",
            "Metallic Madness Zone Act 2",
            "Titanic Monarch Zone Act 1",
            "Titanic Monarch Zone Act 2",
            "Titanic Monarch Zone Boss",
            "Egg Reverie Zone",
            "Special Stage 1",
            "Special Stage 2",
            "Special Stage 3",
            "Special Stage 4",
            "Special Stage 5",
            "Special Stage 6",
            "Special Stage 7",
            "Blue Spheres 1",
            "Blue Spheres 2",
            "Blue Spheres 3",
            "Blue Spheres 4",
            "Blue Spheres 5",
            "Blue Spheres 6",
            "Blue Spheres 7",
            "Blue Spheres 8",
            "Blue Spheres 9",
            "Blue Spheres 10",
            "Blue Spheres 11",
            "Blue Spheres 12",
            "Blue Spheres 13",
            "Blue Spheres 14",
            "Blue Spheres 15",
            "Blue Spheres 16",
            "Blue Spheres 17",
            "Blue Spheres 18",
            "Blue Spheres 19",
            "Blue Spheres 20",
            "Blue Spheres 21",
            "Blue Spheres 22",
            "Blue Spheres 23",
            "Blue Spheres 24",
            "Blue Spheres 25",
            "Blue Spheres 26",
            "Blue Spheres 27",
            "Blue Spheres 28",
            "Blue Spheres 29",
            "Blue Spheres 30",
            "Blue Spheres 31",
            "Blue Spheres 32",
            "Blue Spheres Random",
            "Blue Spheres Random 2",
            "Puyo Puyo",
            "D.A. Garden",
            "Angel Island Zone Cutscene",
            "Green Hill Zone Intro",
            "Green Hill Zone Outro",
            "Mirage Saloon Knuckles Intro",
            "Stardust Speedway Zone Time Warp",
            "Try Again",
            "Bad End - Sonic",
            "Bad End - Tails",
            "Bad End - Knux",
            "Good End",
            "True End?"});
			this.startingScene.Location = new System.Drawing.Point(92, 30);
			this.startingScene.Name = "startingScene";
			this.startingScene.Size = new System.Drawing.Size(191, 21);
			this.startingScene.TabIndex = 2;
			// 
			// enableDebugConsole
			// 
			this.enableDebugConsole.AutoSize = true;
			this.enableDebugConsole.Location = new System.Drawing.Point(7, 7);
			this.enableDebugConsole.Name = "enableDebugConsole";
			this.enableDebugConsole.Size = new System.Drawing.Size(135, 17);
			this.enableDebugConsole.TabIndex = 0;
			this.enableDebugConsole.Text = "Enable Debug Console";
			this.enableDebugConsole.UseVisualStyleBackColor = true;
			// 
			// modContextMenu
			// 
			this.modContextMenu.Items.AddRange(new System.Windows.Forms.ToolStripItem[] {
            this.openFolderToolStripMenuItem});
			this.modContextMenu.Name = "modContextMenu";
			this.modContextMenu.Size = new System.Drawing.Size(138, 26);
			// 
			// openFolderToolStripMenuItem
			// 
			this.openFolderToolStripMenuItem.Name = "openFolderToolStripMenuItem";
			this.openFolderToolStripMenuItem.Size = new System.Drawing.Size(137, 22);
			this.openFolderToolStripMenuItem.Text = "Open folder";
			this.openFolderToolStripMenuItem.Click += new System.EventHandler(this.openFolderToolStripMenuItem_Click);
			// 
			// MainForm
			// 
			this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
			this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
			this.ClientSize = new System.Drawing.Size(384, 409);
			this.Controls.Add(this.tabControl1);
			this.Controls.Add(this.installButton);
			this.Controls.Add(this.saveAndPlayButton);
			this.Controls.Add(this.saveButton);
			this.MinimumSize = new System.Drawing.Size(400, 400);
			this.Name = "MainForm";
			this.Padding = new System.Windows.Forms.Padding(0, 0, 0, 3);
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Show;
			this.Text = "Mania Mod Manager";
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.tabPage1.PerformLayout();
			this.tabPage2.ResumeLayout(false);
			this.tabPage4.ResumeLayout(false);
			this.tabPage4.PerformLayout();
			this.modContextMenu.ResumeLayout(false);
			this.ResumeLayout(false);
			this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.ListView modListView;
        private System.Windows.Forms.ColumnHeader columnHeader1;
		private System.Windows.Forms.ColumnHeader columnHeader2;
        private System.Windows.Forms.Button saveButton;
        private System.Windows.Forms.Button saveAndPlayButton;
		private System.Windows.Forms.Button installButton;
        private System.Windows.Forms.Label modDescription;
		private System.Windows.Forms.Button buttonRefreshModList;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.CheckedListBox codesCheckedListBox;
		private System.Windows.Forms.ColumnHeader columnHeader3;
		private System.Windows.Forms.ToolTip toolTip1;
		private System.Windows.Forms.Button modDownButton;
		private System.Windows.Forms.Button modUpButton;
		private System.Windows.Forms.ContextMenuStrip modContextMenu;
		private System.Windows.Forms.ToolStripMenuItem openFolderToolStripMenuItem;
		private System.Windows.Forms.TabPage tabPage4;
		private System.Windows.Forms.CheckBox enableDebugConsole;
		private System.Windows.Forms.ComboBox startingScene;
	}
}