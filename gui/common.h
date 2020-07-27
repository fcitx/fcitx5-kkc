/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_COMMON_H_
#define _GUI_COMMON_H_

#include <fcitx-utils/misc.h>
#include <glib-object.h>
#include <memory>

namespace fcitx {

template <typename T>
using GObjectUniquePtr = UniqueCPtr<T, g_object_unref>;

template <typename T>
auto makeGObjectUnique(T *p) {
    return GObjectUniquePtr<T>(p);
}

} // namespace fcitx

#endif // _GUI_COMMON_H_
