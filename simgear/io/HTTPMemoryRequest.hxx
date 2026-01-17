// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: 2013 Thomas Geymayer <tomgey@gmail.com>

/**
 * @file
 * @brief HTTP request keeping response in memory.
 */

#pragma once

#include "HTTPRequest.hxx"
#include <fstream>

namespace simgear::HTTP
{

  /**
   * HTTP request keeping response in memory.
   */
  class MemoryRequest:
    public Request
  {
    public:

      /**
       *
       * @param url     Adress to download from
       */
      MemoryRequest(const std::string& url);

      /**
       * Body contents of server response.
       */
      const std::string& responseBody() const;

    protected:
      std::string   _response;

      virtual void responseHeadersComplete();
      virtual void gotBodyData(const char* s, int n);
  };

  typedef SGSharedPtr<MemoryRequest> MemoryRequestRef;

} // namespace simgear::HTTP
