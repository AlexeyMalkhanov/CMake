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
#include "cmCTestSubmitCommand.h"

#include "cmCTest.h"
#include "cmCTestGenericHandler.h"

bool cmCTestSubmitCommand::InitialPass(
  std::vector<std::string> const& args)
{
  const char* res_var = 0;

  bool havereturn_variable = false;
  for(size_t i=0; i < args.size(); ++i)
    {
    if ( havereturn_variable )
      {
      res_var = args[i].c_str();
      havereturn_variable = false;
      }
    else if(args[i] == "RETURN_VALUE")
      {
      if ( res_var )
        {
        this->SetError("called with incorrect number of arguments. "
          "RETURN_VALUE specified twice.");
        return false;
        }
      havereturn_variable = true;
      }
    else
      {
      cmOStringStream str;
      str << "called with incorrect number of arguments. Extra argument is: "
        << args[i].c_str() << ".";
      this->SetError(str.str().c_str());
      return false;
      }
    }

  const char* ctestDropMethod
    = this->Makefile->GetDefinition("CTEST_DROP_METHOD");
  const char* ctestDropSite
    = this->Makefile->GetDefinition("CTEST_DROP_SITE");
  const char* ctestDropLocation
    = this->Makefile->GetDefinition("CTEST_DROP_LOCATION");
  const char* ctestTriggerSite
    = this->Makefile->GetDefinition("CTEST_TRIGGER_SITE");

  if ( !ctestDropMethod )
    {
    ctestDropMethod = "http";
    }
  if ( !ctestDropSite )
    {
    ctestDropSite = "public.kitware.com";
    }
  if ( !ctestDropLocation )
    {
    ctestDropLocation = "/cgi-bin/HTTPUploadDartFile.cgi";
    }
  if ( !ctestTriggerSite )
    {
    ctestTriggerSite
      = "http://public.kitware.com/cgi-bin/Submit-Random-TestingResults.cgi";
    cmCTestLog(this->CTest, HANDLER_OUTPUT, "* Use default trigger site: "
      << ctestTriggerSite << std::endl;);
    }

  this->CTest->SetCTestConfiguration("DropMethod",   ctestDropMethod);
  this->CTest->SetCTestConfiguration("DropSite",     ctestDropSite);
  this->CTest->SetCTestConfiguration("DropLocation", ctestDropLocation);
  this->CTest->SetCTestConfiguration("TriggerSite",  ctestTriggerSite);

  this->CTest->SetCTestConfigurationFromCMakeVariable(this->Makefile,
    "DropSiteUser", "CTEST_DROP_SITE_USER");
  this->CTest->SetCTestConfigurationFromCMakeVariable(this->Makefile,
    "DropSitePassword", "CTEST_DROP_SITE_PASSWORD");
  this->CTest->SetCTestConfigurationFromCMakeVariable(this->Makefile,
    "ScpCommand", "CTEST_SCP_COMMAND");

  const char* notesFilesVariable
    = this->Makefile->GetDefinition("CTEST_NOTES_FILES");
  if (notesFilesVariable)
    {
    std::vector<std::string> notesFiles;
    std::vector<cmStdString> newNotesFiles;
    cmSystemTools::ExpandListArgument(notesFilesVariable,notesFiles);
    std::vector<std::string>::iterator it;
    for ( it = notesFiles.begin();
      it != notesFiles.end();
      ++ it )
      {
      newNotesFiles.push_back(*it);
      }
    this->CTest->GenerateNotesFile(newNotesFiles);
    }
  const char* extraFilesVariable
    = this->Makefile->GetDefinition("CTEST_EXTRA_SUBMIT_FILES");
  if (extraFilesVariable)
    {
    std::vector<std::string> extraFiles;
    std::vector<cmStdString> newExtraFiles;
    cmSystemTools::ExpandListArgument(extraFilesVariable,extraFiles);
    std::vector<std::string>::iterator it;
    for ( it = extraFiles.begin();
      it != extraFiles.end();
      ++ it )
      {
      newExtraFiles.push_back(*it);
      }
    if ( !this->CTest->SubmitExtraFiles(newExtraFiles))
      {
      this->SetError("problem submitting extra files.");
      return false;
      }
    }

  cmCTestGenericHandler* handler
    = this->CTest->GetInitializedHandler("submit");
  if ( !handler )
    {
    this->SetError("internal CTest error. Cannot instantiate submit handler");
    return false;
    }
  int res = handler->ProcessHandler();
  if ( res_var )
    {
    cmOStringStream str;
    str << res;
    this->Makefile->AddDefinition(res_var, str.str().c_str());
    }
  return true;
}


