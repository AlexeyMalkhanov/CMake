/*=========================================================================

  Program:   CMake - Cross-Platform Makefile Generator
  Module:    $RCSfile$
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 2002 Kitware, Inc., Insight Consortium.  All rights reserved.
  See Copyright.txt or http://www.cmake.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "cmVTKWrapJavaCommand.h"

// cmVTKWrapJavaCommand
bool cmVTKWrapJavaCommand::InitialPass(std::vector<std::string> const& argsIn)
{
  if(argsIn.size() < 3 )
    {
    this->SetError("called with incorrect number of arguments");
    return false;
    }
  std::vector<std::string> args;
  this->Makefile->ExpandSourceListArguments(argsIn, args, 2);

  // Now check and see if the value has been stored in the cache
  // already, if so use that value and don't look for the program
  if(!this->Makefile->IsOn("VTK_WRAP_JAVA"))
    {
    return true;
    }

  // what is the current source dir
  std::string cdir = this->Makefile->GetCurrentDirectory();

  // keep the library name
  this->LibraryName = args[0];
  this->SourceList = args[1];
  std::string sourceListValue;
  // was the list already populated
  const char *def = this->Makefile->GetDefinition(this->SourceList.c_str());  
  if (def)
    {
    sourceListValue = def;
    }
  
  // Prepare java dependency file
  const char* resultDirectory = 
          this->Makefile->GetRequiredDefinition("VTK_JAVA_HOME");
  std::string res = this->Makefile->GetCurrentOutputDirectory();

  std::string depFileName = res + "/JavaDependencies.cmake";
  std::ofstream depFile(depFileName.c_str());
  depFile << "# This file is automatically generated by CMake VTK_WRAP_JAVA" 
          << std::endl << std::endl;
  depFile << "SET(VTK_JAVA_DEPENDENCIES ${VTK_JAVA_DEPENDENCIES}" << std::endl;
  
  // get the list of classes for this library
  for(std::vector<std::string>::const_iterator j = (args.begin() + 2);
      j != args.end(); ++j)
    {   
    cmSourceFile *curr = this->Makefile->GetSource(j->c_str());

    // if we should wrap the class
    if (!curr || !curr->GetPropertyAsBool("WRAP_EXCLUDE"))
      {
      cmSourceFile file;
      if (curr)
        {
        file.SetProperty("ABSTRACT",curr->GetProperty("ABSTRACT"));
        }
      std::string srcName = cmSystemTools::GetFilenameWithoutExtension(*j);
      std::string newName = srcName + "Java";
      file.SetName(newName.c_str(), this->Makefile->GetCurrentOutputDirectory(),
                   "cxx",false);
      std::string hname = cdir + "/" + srcName + ".h";
      this->WrapHeaders.push_back(hname);
      // add starting depends
      file.GetDepends().push_back(hname);
      this->WrapClasses.push_back(file);
      this->OriginalNames.push_back(srcName);
      if (sourceListValue.size() > 0)
        {
        sourceListValue += ";";
        }
      sourceListValue += newName + ".cxx";

      // Write file to java dependency file
      std::string jafaFile = resultDirectory;
      jafaFile += "/";
      jafaFile += srcName;
      jafaFile += ".java";
      depFile << "  " << jafaFile << std::endl;
      }
    }

  // Finalize java dependency file
  depFile << ")" << std::endl;
  
  this->Makefile->AddDefinition(this->SourceList.c_str(), sourceListValue.c_str());  
  return true;
}

void cmVTKWrapJavaCommand::FinalPass() 
{
  // first we add the rules for all the .h to Java.cxx files
  size_t lastClass = this->WrapClasses.size();
  std::vector<std::string> depends;
  std::vector<std::string> depends2;
  std::vector<std::string> alldepends;  
  const char* wjava = 
    this->Makefile->GetRequiredDefinition("VTK_WRAP_JAVA_EXE");
  const char* pjava = 
    this->Makefile->GetRequiredDefinition("VTK_PARSE_JAVA_EXE");
  const char* hints = this->Makefile->GetDefinition("VTK_WRAP_HINTS");
  const char* resultDirectory = 
    this->Makefile->GetRequiredDefinition("VTK_JAVA_HOME");

  // wrap all the .h files
  depends.push_back(wjava);
  depends2.push_back(pjava);
  if(hints)
    {
    depends.push_back(hints);
    depends2.push_back(hints);
    }
  for(size_t classNum = 0; classNum < lastClass; classNum++)
    {
    this->Makefile->AddSource(this->WrapClasses[classNum]);

    // wrap java
    std::string res = this->Makefile->GetCurrentOutputDirectory();
    res += "/";
    res += this->WrapClasses[classNum].GetSourceName() + ".cxx";
    std::string res2 = resultDirectory;
    res2 += "/";
    res2 += this->OriginalNames[classNum];
    res2 += ".java";
    
    cmCustomCommandLine commandLineW;
    commandLineW.push_back(wjava);
    commandLineW.push_back(this->WrapHeaders[classNum]);
    if(hints)
      {
      commandLineW.push_back(hints);
      }
    commandLineW.push_back((this->WrapClasses[classNum].
                            GetPropertyAsBool("ABSTRACT") ? "0" : "1"));
    commandLineW.push_back(res);

    cmCustomCommandLines commandLines;
    commandLines.push_back(commandLineW);
    std::vector<std::string> outputs;
    outputs.push_back(res);
    const char* no_comment = 0;
    this->Makefile->AddCustomCommandOldStyle(this->LibraryName.c_str(),
                                         outputs,
                                         depends,
                                         this->WrapHeaders[classNum].c_str(),
                                         commandLines,
                                         no_comment);

    cmCustomCommandLine commandLineP;
    commandLineP.push_back(pjava);
    commandLineP.push_back(this->WrapHeaders[classNum]);
    if(hints)
      {
      commandLineP.push_back(hints);
      }
    commandLineP.push_back((this->WrapClasses[classNum].
                            GetPropertyAsBool("ABSTRACT") ? "0" : "1"));
    commandLineP.push_back(res2);

    cmCustomCommandLines commandLines2;
    commandLines2.push_back(commandLineP);
    std::vector<std::string> outputs2;
    outputs2.push_back(res2);
    this->Makefile->AddCustomCommandOldStyle(this->LibraryName.c_str(),
                                         outputs2,
                                         depends2,
                                         this->WrapHeaders[classNum].c_str(),
                                         commandLines2,
                                         no_comment);
    alldepends.push_back(res2);
    }

  const char* no_output = 0;
  const char* no_working_directory = 0;
  this->Makefile->AddUtilityCommand((this->LibraryName+"JavaClasses").c_str(),
                                true, no_output, 
                                alldepends, no_working_directory, "");
}
