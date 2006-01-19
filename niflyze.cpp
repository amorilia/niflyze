/* Copyright (c) 2005, NIF File Format Library and Tools
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:

   * Redistributions of source code must retain the above copyright
     notice, this list of conditions and the following disclaimer.

   * Redistributions in binary form must reproduce the above
     copyright notice, this list of conditions and the following
     disclaimer in the documentation and/or other materials provided
     with the distribution.

   * Neither the name of the NIF File Format Library and Tools
     project nor the names of its contributors may be used to endorse
     or promote products derived from this software without specific
     prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE. */

#include "niflib.h"
#include <iomanip>
#include <iostream>
#include <vector>
#include <algorithm>
#include <cmath>

// _WIN32 will detect windows on most compilers
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#else
#include <dirent.h>
#include <regex.h>
#include <time.h>
#endif

using namespace std;

#define endl "\r\n"

//vector<string> dark_tx;
//vector<string> detail_tx;
//vector<string> decal_tx;
//vector<string> decal2_tx;
//vector<string> glow_tx;
//vector<string> gloss_tx;

int main( int argc, char* argv[] );
bool HasBlockType( vector<blk_ref> blocks, string const & block_type );
void PrintHelpInfo( ostream & out );
void PrintTree( blk_ref block, int indent, ostream & out );

int main( int argc, char* argv[] ){
	bool block_match = false;
	bool exclusive_mode = false;
	bool use_start_dir = false;
	char * block_match_string = "";
	char * in_file = "*.nif";  //C_Templar_M_G_skirt
	char * out_file = "niflyze.txt";
	char * start_dir = "";

	////Temporary//
	//start_dir = "C:\\Documents and Settings\\Shon\\My Documents\\Modding\\Morrowind\\Official Nifs";
	//use_start_dir = true;
	///////////////

	bool help_flag = false;
	for (int i = 0; i < argc; ++i ) {
		//--Look for switches--//

		// Input File
		if ( strcmp(argv[i], "-i") == 0 ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record input file name
				i++;
				in_file = argv[i];
			}
		}

		// Output File
		if ( strcmp(argv[i], "-o") == 0  ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record output file name
				i++;
				
				out_file = argv[i];
			}
		}

		// Start Directory
		if ( strcmp(argv[i], "-p") == 0  ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record output file name
				i++;
				start_dir = argv[i];
				use_start_dir = true;
			}
		}

		// Block Match
		if ( strcmp(argv[i], "-b") == 0  ) {
			
			//If not already on the last argument
			if (i != argc - 1) {
				//Move to next argument and record output file name
				i++;
				block_match_string = argv[i];
				block_match = true;
			}
		}

		// Verbose mode
		if ( strcmp(argv[i], "-v") == 0  ) {
			
			SetVerboseMode(true);
		}

		// Exclusive mode
		if ( strcmp(argv[i], "-x") == 0  ) {
			exclusive_mode = true;
		}

		// Help
		if ( strcmp(argv[i], "-?") == 0  ) {
			help_flag = true;
		}
	}	

	// if there are no arguments, show help
	if ( argc == 1 ) {
		help_flag = true;
	}

	if ( help_flag == true ) {
		//Help request intercepted
		PrintHelpInfo( cout );
		return 1;
	}

#ifdef _WIN32
	WIN32_FIND_DATA FindFileData;
	HANDLE hFind;
#else
	DIR *dir;
	dirent *pdirent;
	string pattern(in_file);
	string tmp;
	for ( string::const_iterator it = pattern.begin(); it != pattern.end(); it++) {
		if ( *it == '.' )
			tmp += "\\.";
		else if ( *it == '*' )
			tmp += ".*";
		else tmp += *it;
	};
	pattern = tmp + "$";
	
	regex_t regex;
	if (regcomp(&regex, pattern.c_str(), REG_EXTENDED|REG_NOSUB) == -1)
		cout << "Failed to compile regular expression." << endl;
#endif

	//Open output file
	ofstream out( out_file, ofstream::binary );
	out.setf(ios::fixed, ios::floatfield);
	out << setprecision(1);
	cout.setf(ios::fixed, ios::floatfield);
	cout << setprecision(2);

#ifdef _WIN32
	if ( use_start_dir )
		SetCurrentDirectory(start_dir);
#else
	dir = opendir(start_dir);
	if (dir == NULL) {
		cout << "Directory not found." << endl;
		return -1;
	};
#endif

	//Start Timer
#ifdef _WIN32
	DWORD start_time = GetTickCount();
#else
	clock_t start_time = clock();
#endif
	unsigned int count = 0;

	map<string, vector<string> > versions;

	// Find files
#ifdef _WIN32
	hFind = FindFirstFile(in_file, &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE) {
#else
	pdirent = readdir(dir);
	while (pdirent != NULL) {
		if (strcmp(".", pdirent->d_name) == 0 || strcmp("..", pdirent->d_name) == 0) {
			pdirent = readdir(dir);
			continue;
		};
		if (regexec(&regex, pdirent->d_name, 0, NULL, 0) == 0) break;
		pdirent = readdir(dir);
    };
	if (pdirent == NULL) {
#endif
		cout << "No files found." << endl;
		return 1;
	} else {

		bool file_found = true;
		while (file_found == true) {
			//--Read NIF File--//
#ifdef _WIN32
			string current_file = FindFileData.cFileName;
#else
			string current_file = string(start_dir) + "/" + string(pdirent->d_name);
#endif
			cout << "Reading " << current_file << "...";

			//--Open File--//
			ifstream in( current_file.c_str(), ifstream::binary );

			//--Read Header--//
			
			char header_string[256];
			in.getline( header_string, 256 );
			unsigned int version;
			in.read( (char*) &version, 4);
			in.close();

			if ( version < VER_4_0_0_2 || version > VER_10_0_1_0 ) {
				cout << "Unsupported Version:  " << header_string << endl;

				//--Find Next File--//
#ifdef _WIN32
				file_found = FindNextFile(hFind, &FindFileData);
#else
				pdirent = readdir(dir);
				while (pdirent != NULL) {
					if (regexec(&regex, pdirent->d_name, 0, NULL, 0) == 0) break;
					pdirent = readdir(dir);
				};
				if (pdirent == NULL) file_found = false; else file_found = true;
#endif

				continue;
			}

			//versions[header_string].push_back(current_file);

			//out << setw(30) << current_file << ":  " << header_string << endl;


			

			vector< blk_ref > blocks;
			blk_ref root;
			try {
				////Show block tree
				//root = ReadNifTree( current_file );
				//PrintTree( root, 0, out );
				//out << endl;

				blocks = ReadNifList( current_file );
				//blocks.push_back( ReadNifTree( current_file ) );


				////Add files to the lists if they have that specific type of texture
				//for ( unsigned int i = 0; i < blocks.size(); ++i ) {
				//	if ( blocks[i]->GetBlockType() == "NiTexturingProperty" ) {
				//		//if ( blocks[i]->GetAttr("Dark Texture")->asTexture().isUsed == true ) {
				//		//	dark_tx.push_back( current_file );
				//		//}
				//		//if ( blocks[i]->GetAttr("Detail Texture")->asTexture().isUsed == true ) {
				//		//	detail_tx.push_back( current_file );
				//		//}
				//		//if ( blocks[i]->GetAttr("Decal Texture")->asTexture().isUsed == true ) {
				//		//	decal_tx.push_back( current_file );
				//		//}
				//		//if ( blocks[i]->GetAttr("Decal Texture 2")->asTexture().isUsed == true ) {
				//		//	decal2_tx.push_back( current_file );
				//		//}
				//		//if ( blocks[i]->GetAttr("Glow Texture")->asTexture().isUsed == true ) {
				//		//	glow_tx.push_back( current_file );
				//		//}
				//		//if ( blocks[i]->GetAttr("Gloss Texture")->asTexture().isUsed == true ) {
				//		//	gloss_tx.push_back( current_file );
				//		//}

				//		//Try to copy the base texture to another slot
				//Texture t = blocks[i]["Base Texture"]->asTexture();
				//		float pi = 3.141592653589793f;
				//		float angle = 0.25f * pi;
				//		
				//		t.bmLumaOffset = 45.0f;
				//		t.bmLumaScale = 45.0f;
				//		t.bmMatrix[0][0] = cos(angle);		t.bmMatrix[0][1] = -sin(angle);
				//		t.bmMatrix[1][0] = sin(angle);		t.bmMatrix[1][1] = cos(angle);
				//		blocks[i]["Broken Texture?"]->Set(t);

				//		//Create a texture source for the alpha block
				//		blk_ref src_prop = CreateBlock( "NiSourceTexture" );
				//		TextureSource ts;
				//		ts.useExternal = true;
				//		ts.fileName = "snowwallbump.tga";
				//		ts.unknownByte = 0;
				//		src_prop["Texture Source"]->Set(ts);
				//		src_prop["Pixel Layout"]->Set(4);
				//		src_prop["Use Mipmaps"]->Set(0);
				//		src_prop["Alpha Format"]->Set(0);
				//		src_prop["Unknown Byte"]->Set(1);
				//		blk_link l;
				//		l.attr = blocks[i]["Broken Texture?"];
				//		l.block = src_prop;
				//		blocks[i]->AddLink( l );
				//		
				//		//t.isUsed = false;
				//		//blocks[i]->GetAttr("Base Texture")->Set(t);

				//		//blk_link l = blocks[i]->GetLink( blocks[i]->GetAttr("Base Texture") );
				//		//l.attr = blocks[i]->GetAttr("Broken Texture?");
				//		//blocks[i]->AddLink( l );
				//	}
				//	//if ( blocks[i]->GetBlockType() == "NiSourceTexture" ) {
				//	//	blocks[i]->GetAttr("Pixel Layout")->Set(4);
				//	//}
				//	if ( blocks[i]->GetBlockType() == "NiMaterialProperty" ) {
				//		blocks[i]["Specular Color"]->Set( 1.0f, 1.0f, 1.0f );
				//		blocks[i]["Glossiness"]->Set( 20.0f );
				//	}
				//	if ( blocks[i]->GetBlockType() == "NiTriShape" ) {
				//		blk_ref spec_prop = CreateBlock( "NiSpecularProperty" );
				//		spec_prop["Flags"]->Set(1);
				//		blk_link l;
				//		l.attr = blocks[i]["Properties"];
				//		l.block = spec_prop;
				//		blocks[i]->AddLink( l );

				//		blk_ref render_prop = CreateBlock( "NiRendererSpecificProperty" );
				//		render_prop["Flags"]->Set(int(0xFFFFFFFF));
				//		l.attr = blocks[i]["Properties"];
				//		l.block = render_prop;
				//		blocks[i]->AddLink( l );
				//	}

				//}

				blocks = ReadNifList( current_file );
				//blocks.push_back( ReadNifTree( current_file ) );

				//Increment file count
				count++;

				//--Output Analysis--//
				if ( !block_match || HasBlockType( blocks, string(block_match_string) ) ) {
					cout << "writing...";
					if (exclusive_mode) {
						for ( unsigned int i = 0; i < blocks.size(); ++i ) {
						if ( blocks[i]->GetBlockType() == string(block_match_string) ) {
							out << "====[ " << current_file << " | Block " << blocks[i].get_index() << " | " << blocks[i]->GetBlockType() << " ]====" << endl
								<< blocks[i]->asString()
								<< endl;
						}
					}
					} else {
						for ( unsigned int i = 0; i < blocks.size(); ++i ) {
							out << "====[ " << current_file << " | Block " << blocks[i].get_index() << " | " << blocks[i]->GetBlockType() << " ]====" << endl
								<< blocks[i]->asString()
								<< endl;
						}
					}
				}

				cout << "done" << endl;
			}
			catch( exception & e ) {
				cout << "Error: " << e.what() << endl;
				cout << "\a";
				return 0;
			}
			catch( ... ) {
				cout << "\nUnknown Exception." << endl;
				return 0;
			}

			////Test Write Function
			//string output_nif_file = "C:\\Documents and Settings\\Shon\\My Documents\\Visual Studio Projects\\Niflyze\\Release\\TEST.NIF";
			//WriteNifTree( output_nif_file, blocks[0] );

			//Clear out current file
			blocks.clear();

			//--Find Next File--//
#ifdef _WIN32
			file_found = FindNextFile(hFind, &FindFileData);
#else
			pdirent = readdir(dir);
			while (pdirent != NULL) {
				if (regexec(&regex, pdirent->d_name, 0, NULL, 0) == 0) break;
				pdirent = readdir(dir);
			};
			if (pdirent == NULL) file_found = false; else file_found = true;
#endif
		}
	}

	////Print out file/version correspondance
	//map<string, vector<string> >::iterator it;
	//for (it = versions.begin(); it != versions.end(); ++it ) {
	//	out << endl
	//		<< "===[" << it->first << "]===" << endl;

	//	vector<string>::iterator it2;
	//	for (it2 = it->second.begin(); it2 != it->second.end(); ++it2) {
	//		out << "   " << *it2 << endl;
	//	}

	//}
	
	////Prune list
	//sort< vector<string>::iterator >(versions.begin(), versions.end());
	//unique<  vector<string>::iterator  >(versions.begin(), versions.end());

		////Print out list
	//out << "Versions:" << endl;
	//for ( unsigned int i = 0; i < versions.size(); ++i ) {
	//	out << "   " << versions[i] << endl;
	//}

	////Prune lists
	//sort(dark_tx.begin(), dark_tx.end());
	//unique(dark_tx.begin(), dark_tx.end());

	//sort(detail_tx.begin(), detail_tx.end());
	//unique(detail_tx.begin(), detail_tx.end());

	//sort(decal_tx.begin(), decal_tx.end());
	//unique(decal_tx.begin(), decal_tx.end());

	//sort(decal2_tx.begin(), decal2_tx.end());
	//unique(decal2_tx.begin(), decal2_tx.end());

	//sort(glow_tx.begin(), glow_tx.end());
	//unique(glow_tx.begin(), glow_tx.end());

	//sort(gloss_tx.begin(), gloss_tx.end());
	//unique(gloss_tx.begin(), gloss_tx.end());

	////Print out lists

	//out << "Dark Texture:" << endl;
	//for (unsigned int i = 0; i < dark_tx.size(); ++i) {
	//	out << "   " << dark_tx[i] << endl;
	//}
	//out << endl;
	//out << "Detail Texture:" << endl;
	//for (unsigned int i = 0; i < detail_tx.size(); ++i) {
	//	out << "   " << detail_tx[i] << endl;
	//}
	//out << endl;
	//out << "Decal Texture:" << endl;
	//for (unsigned int i = 0; i < decal_tx.size(); ++i) {
	//	out << "   " << decal_tx[i] << endl;
	//}
	//out << "Decal Texture 2:" << endl;
	//for (unsigned int i = 0; i < decal2_tx.size(); ++i) {
	//	out << "   " << decal2_tx[i] << endl;
	//}
	//out << endl;
	//out << "Glow Texture:" << endl;
	//for (unsigned int i = 0; i < glow_tx.size(); ++i) {
	//	out << "   " << glow_tx[i] << endl;
	//}
	//out << endl;
	//out << "Gloss Texture:" << endl;
	//for (unsigned int i = 0; i < gloss_tx.size(); ++i) {
	//	out << "   " << gloss_tx[i] << endl;
	//}

	out.close();

	cout << "Done!" << endl;
#ifdef _WIN32
	DWORD end_time = GetTickCount();
	float time_taken = float(end_time - start_time) / 1000.0f;
#else
	closedir(dir);
	clock_t end_time = clock();
	float time_taken = float(end_time - start_time) / CLOCKS_PER_SEC;
#endif
	cout << count << " files analyzed." << endl
		 << "Total processing time:  " << time_taken << " seconds" << endl; 
	return 1;
}

bool HasBlockType( vector<blk_ref> blocks, string const & block_type ) {
	for ( unsigned int i = 0; i < blocks.size(); ++i ) {
		if ( blocks[i]->GetBlockType() == block_type )
			return true;
	}
	return false;
}

void PrintTree( blk_ref block, int indent, ostream & out ) {
	//Print indent
	for (int i = 0; i < indent; ++i) {
		out << "   ";
	}

	//Print Block
	out << "* " << block << endl;

	//Call this function for all children of this block with a higher indent
	list<blk_ref> links = block->GetLinks();
	list<blk_ref>::iterator it;
	for ( it = links.begin(); it != links.end(); ++it ) {
		if ( (*it)->GetParent() == block )
			PrintTree( *it, indent + 1, out );
	}
}

void PrintHelpInfo( ostream & out ) {
	out << "Usage:  niflyze -switch_1 value_1 -switch_2 value_2 ... switch_n value_n" << endl
		<< "Example:  niflyze -i model.nif -o output.txt" << endl << endl
		<< "Switches:" << endl
		<< "-? [Help]" << endl
		<< "   Displays This Information" << endl << endl
		<< "-i [Input File]" << endl
		<< "   The nif file(s) to analyze.  Can include wildcards." << endl
		<< "   Example: model.nif" << endl
		<< "   Default: *.nif" << endl << endl
		<< "-o [Output File]" << endl
		<< "   The text file to output analysis of NIF file(s) to." << endl
		<< "   Example: -o output.txt" << endl
		<< "   Default:  niflyze.txt" << endl << endl
		<< "-p [Search Path]" << endl
		<< "   Specifies the directory to search for files matching the input file name." << endl
		<< "   If no path is specified, the present directory is searched.  Don't specify" << endl
		<< "   a path in the in file or the read will fail." << endl
		<< "   Example:  -p \"C:\\Program Files\\\"" << endl << endl
		<< "-b [Block Match]" << endl
		<< "   Causes niflyze to output information only for files which contain the" << endl
		<< "   specified block type. Default behavior is to output information about" << endl
		<< "   all files read." << endl
		<< "   Example: -b NiNode" << endl << endl
		<< "-v [Verbose]" << endl
		<< "   Causes niflyze to output all complete data arrays such as vertices and" << endl
		<< "   unknown data areas in hex form. Default behavior is not to output this" << endl
		<< "   information." << endl
		<< "   Example: -v" << endl << endl
		<< "-x [Exclusive Mode]" << endl
		<< "   Causes niflyze to output only the blocks that match the block type" << endl
		<< "   specified with the -b switch.  Normally the whole file that contians" << endl
		<< "   the block is output." << endl
		<< "   Example: -x" << endl;
}
