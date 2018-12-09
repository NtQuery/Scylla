using System;
using System.Xml;
using System.IO;

using System.Windows.Forms;

namespace ScyllaToImprecTree
{
	public partial class Form1 : Form
	{
		public Form1()
		{
			InitializeComponent();
		}

		private string _scyllaXmlPath = @"";
		private string _imprecFilePath = @"";

		private void button1_Click(object sender, EventArgs e)
		{
			StreamWriter outputfile = new System.IO.StreamWriter(_imprecFilePath);
			XmlDocument doc = new XmlDocument();
			doc.Load(_scyllaXmlPath);

			XmlNodeList nodes = doc.DocumentElement.SelectNodes("/target/module");

			string filename = doc.DocumentElement.Attributes["filename"].Value;
			string oepVa = doc.DocumentElement.Attributes["oep_va"].Value;
			string iatVa = doc.DocumentElement.Attributes["iat_va"].Value;
			string iatSize = doc.DocumentElement.Attributes["iat_size"].Value;

			outputfile.WriteLine("Target: " + filename);
			outputfile.WriteLine("OEP: " + oepVa + "\tIATRVA: " + iatVa + "\tIATSize: " + iatSize);
			outputfile.WriteLine();

			foreach (XmlNode node in nodes)
			{
				string moduleFilename = node.Attributes["filename"].Value;
				string firstThunkRva = node.Attributes["first_thunk_rva"].Value;

				XmlNodeList nodesImports = node.SelectNodes("import_valid");
				XmlNodeList nodesImportsInvalid = node.SelectNodes("import_invalid");

				int sum = nodesImports.Count + nodesImportsInvalid.Count;

				outputfile.WriteLine("FThunk: " + firstThunkRva + "\tNbFunc: " + sum.ToString("X"));

				foreach (XmlNode import in nodesImports)
				{
					string importname = import.Attributes["name"].Value;
					string importordinal = import.Attributes["ordinal"].Value;
					string importrva = import.Attributes["iat_rva"].Value;

					outputfile.WriteLine("1\t" + importrva + "\t" + moduleFilename + "\t" + importordinal + "\t" + importname);
				}

				foreach (XmlNode import in nodesImportsInvalid)
				{
					string importrva = import.Attributes["iat_rva"].Value;
					string address_va = import.Attributes["address_va"].Value;
					outputfile.WriteLine("0\t" + importrva + "\t" + moduleFilename + "\t" + 0 + "\t" + address_va);
				}

				outputfile.WriteLine();
			}

			outputfile.Close();
		}

		private void textBox1_TextChanged(object sender, EventArgs e)
		{

		}

		private void textBox1_Enter(object sender, EventArgs e)
		{
			openOpenFileDialog();
		}

		private void textBox1_MouseClick(object sender, MouseEventArgs e)
		{
			openOpenFileDialog();
		}

		private void openOpenFileDialog()
		{
			string path = string.Empty;

			OpenFileDialog openFileDialog1 = new OpenFileDialog();
			openFileDialog1.Filter = "xml files (*.xml)|*.xml|All files (*.*)|*.*";
			if (openFileDialog1.ShowDialog() == DialogResult.OK)
			{
				textBox1.Text = openFileDialog1.FileName;
				_scyllaXmlPath = openFileDialog1.FileName;
			}
		}

		private void saveFile()
		{
			SaveFileDialog saveFileDialog1 = new SaveFileDialog();
			saveFileDialog1.Title = "Save ImpREC File";
			saveFileDialog1.CheckPathExists = true;
			saveFileDialog1.DefaultExt = "txt";
			saveFileDialog1.Filter = "Text files (*.txt)|*.txt|All files (*.*)|*.*";
			saveFileDialog1.FilterIndex = 2;
			saveFileDialog1.RestoreDirectory = true;

			if (saveFileDialog1.ShowDialog() == DialogResult.OK)
			{
				textBox2.Text = saveFileDialog1.FileName;
				_imprecFilePath = saveFileDialog1.FileName;
			}
		}

		private void textBox2_Enter(object sender, EventArgs e)
		{
			saveFile();
		}

		private void textBox2_Click(object sender, EventArgs e)
		{
			saveFile();
		}
	}
}
