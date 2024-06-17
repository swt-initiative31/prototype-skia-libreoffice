# -*- Mode: makefile-gmake; tab-width: 4; indent-tabs-mode: t -*-
#
# This file is part of the LibreOffice project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
#
# This file incorporates work covered by the following license notice:
#
#   Licensed to the Apache Software Foundation (ASF) under one or more
#   contributor license agreements. See the NOTICE file distributed
#   with this work for additional information regarding copyright
#   ownership. The ASF licenses this file to you under the Apache
#   License, Version 2.0 (the "License"); you may not use this file
#   except in compliance with the License. You may obtain a copy of
#   the License at http://www.apache.org/licenses/LICENSE-2.0 .
#

$(eval $(call gb_Library_Library,vcl4swt))



$(eval $(call gb_Library_set_include,vcl4swt,\
    $$(INCLUDE) \
    -I$(SRCDIR)/vcl4swt/inc \
))

$(eval $(call gb_Library_add_defs,vcl4swt,\
    -DVCL_DLLIMPLEMENTATION \
    -DDLLIMPLEMENTATION_UITEST \
    -DCUI_DLL_NAME=\"$(call gb_Library_get_runtime_filename,$(call gb_Library__get_name,cui))\" \
    -DTK_DLL_NAME=\"$(call gb_Library_get_runtime_filename,$(call gb_Library__get_name,tk))\" \
    $(if $(SYSTEM_LIBFIXMATH),-DSYSTEM_LIBFIXMATH) \
))

$(eval $(call gb_Library_use_sdk_api,vcl4swt))

$(eval $(call gb_Library_use_libraries,vcl4swt,\
    tl \
    sal \
    vcl \
    cppu \
    cppuhelper \
    comphelper \
    i18nlangtag \
    fwk \
))


$(eval $(call gb_Library_use_externals,vcl4swt,\
    boost_headers \
    expat \
    frozen \
    gio \
    graphite \
    harfbuzz \
    icu_headers \
    icuuc \
    lcms2 \
    libeot \
    libjpeg \
    libpng \
    libtiff \
    libwebp \
))
$(eval $(call gb_Library_add_exception_objects,vcl4swt,\
    vcl4swt/src/api/swtapi \
))


# vim: set noet sw=4 ts=4:
