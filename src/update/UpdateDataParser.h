/*!********************************************************************
 Audacity: A Digital Audio Editor

 @file UpdateDataParser.h
 @brief Declare a class that parses update server data format.

 Anton Gerasimov
 **********************************************************************/
#pragma once

#include "VersionPatch.h"

#include "xml/XMLTagHandler.h"

#include <wx/arrstr.h>
#include <map>

/// A class that parses update server data format.
class UpdateDataParser final : public XMLTagHandler
{
public:
    UpdateDataParser();
    ~UpdateDataParser();

    //! Parsing from update data format to VersionPatch fields.
    /*!
       @param updateData InputData.
       @param versionPath Parsed output data.
       @return True if success.
    */
    bool Parse(const VersionPatch::UpdateDataFormat& updateData, VersionPatch* versionPatch);

private:
    enum class XmlParsedTags : int {
        kNotUsedTag,
        kUpdateTag,
        kDescriptionTag,
        kOsTag,
        kWindowsTag,
        kMacosTag,
        kLinuxTag,
        kVersionTag,
        kLinkTag
    };
    XmlParsedTags mXmlParsingState{ XmlParsedTags::kNotUsedTag };

    std::map<XmlParsedTags, wxString> mXmlTagNames{
        { XmlParsedTags::kUpdateTag, L"Updates" },
        { XmlParsedTags::kDescriptionTag, L"Description" },
        { XmlParsedTags::kOsTag, L"OS" },
        { XmlParsedTags::kWindowsTag, L"Windows" },
        { XmlParsedTags::kMacosTag, L"Macos" },
        { XmlParsedTags::kLinuxTag, L"Linux" },
        { XmlParsedTags::kVersionTag, L"Version" },
        { XmlParsedTags::kLinkTag, L"Link" },
    };

    bool HandleXMLTag(const wxChar* tag, const wxChar** attrs) override;
    void HandleXMLEndTag(const wxChar* tag) override;
    void HandleXMLContent(const wxString& content) override;
    XMLTagHandler* HandleXMLChild(const wxChar* tag) override;

    wxArrayString SplitChangelogSentences(const wxString& changelogContent);

    VersionPatch* mVersionPatch{ nullptr };
};
