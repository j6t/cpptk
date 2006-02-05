#include "cpptk.h"
#include <iostream>
#include <fstream>

using namespace Tk;
using namespace std;

// this procedure will support the "File->Open" menu command
void openFile()
{
     // open standard "Open File" dialog
     
     string fileName(tk_getOpenFile());
     
     // read the file
     
     ifstream f(fileName.c_str());
     string fileContent(
          istreambuf_iterator<char>(f.rdbuf()),
          istreambuf_iterator<char>());
     
     // put the file content into the text widget
     
     ".t" << deletetext(txt(1,0), end);
     ".t" << insert(end, fileContent);
}

// this procedure will support the "File->Save" menu command
void saveFile()
{
     // open standard "Save File" dialog
     
     string fileName(tk_getSaveFile());
     
     // get the content from the text widget
     
     string content(".t" << get(txt(1,0), end));
     
     // write the file
     
     ofstream f(fileName.c_str());
     f << content;
}

int main(int, char *argv[])
{
     try
     {
          init(argv[0]);
          
          // create the menubar

          frame(".mbar") -borderwidth(1) -relief(raised);
          pack(".mbar") -Tk::fill(x);
          
          // create the menu File entry
          
          menubutton(".mbar.file") -text("File") -submenu(".mbar.file.m");
          pack(".mbar.file") -side(Tk::left);
          
          // create the drop-down menu
          
          string drop(menu(".mbar.file.m"));
          drop << add(command) -menulabel("Open") -command(openFile);
          drop << add(command) -menulabel("Save") -command(saveFile);
          drop << add(command) -menulabel("Exit") -command(string("exit"));
          
          // create the text widget with its scrollbar
          
          textw(".t") -background("white") -yscrollcommand(".s set");
          scrollbar(".s") -command(string(".t yview"));
          pack(".s") -side(Tk::right) -Tk::fill(y);
          pack(".t") -expand(true) -Tk::fill(both);
          
          runEventLoop();
     }
     catch (exception const &e)
     {
          cerr << "Error: " << e.what() << '\n';
     }
}
