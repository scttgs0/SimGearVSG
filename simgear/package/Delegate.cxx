// SPDX-License-Identifier: LGPL-2.1-or-later
// SPDX-FileCopyrightText: Copyright (C) 2017  James Turner - james@flightgear.org


#include <simgear/package/Catalog.hxx>
#include <simgear/package/Delegate.hxx>
#include <simgear/package/Install.hxx>
#include <simgear/package/Package.hxx>

namespace simgear::pkg {

void Delegate::installStatusChanged(InstallRef aInstall, StatusCode aReason)
{
}

void Delegate::dataForThumbnail(const std::string& aThumbnailUrl,
                                size_t lenth, const uint8_t* bytes)
{
}

} // namespace simgear::pkg
