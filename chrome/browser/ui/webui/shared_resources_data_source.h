// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_UI_WEBUI_SHARED_RESOURCES_DATA_SOURCE_H_
#define CHROME_BROWSER_UI_WEBUI_SHARED_RESOURCES_DATA_SOURCE_H_

#include <string>

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "content/public/browser/url_data_source.h"

// A DataSource for chrome://resources/ URLs.
class SharedResourcesDataSource : public content::URLDataSource {
 public:
  SharedResourcesDataSource();

  // content::URLDataSource implementation.
  virtual std::string GetSource() OVERRIDE;
  virtual void StartDataRequest(
      const std::string& path,
      bool is_incognito,
      const content::URLDataSource::GotDataCallback& callback) OVERRIDE;
  virtual std::string GetMimeType(const std::string&) const OVERRIDE;

 private:
  virtual ~SharedResourcesDataSource();

  DISALLOW_COPY_AND_ASSIGN(SharedResourcesDataSource);
};

#endif  // CHROME_BROWSER_UI_WEBUI_SHARED_RESOURCES_DATA_SOURCE_H_
