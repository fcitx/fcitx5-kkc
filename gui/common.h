/*
 * SPDX-FileCopyrightText: 2017~2017 CSSlayer <wengxt@gmail.com>
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 */
#ifndef _GUI_COMMON_H_
#define _GUI_COMMON_H_

#include <glib-object.h>
#include <memory>

namespace fcitx {

template <typename T>
using GObjectUniquePtr = std::unique_ptr<T, decltype(&g_object_unref)>;

template <typename T>
GObjectUniquePtr<T> makeGObjectUnique(T *p) {
    return {p, &g_object_unref};
}

} // namespace fcitx

#endif // _GUI_COMMON_H_
