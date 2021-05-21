// Copyright (c) 2018 Marshall A. Greenblatt. All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//    * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//    * Neither the name of Google Inc. nor the name Chromium Embedded
// Framework nor the names of its contributors may be used to endorse
// or promote products derived from this software without specific prior
// written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// ---------------------------------------------------------------------------
//
// This file was generated by the CEF translator tool and should not edited
// by hand. See the translator.README.txt file in the tools directory for
// more information.
//
// $hash=7922547ef740e63eb30c7c309f638c0f4921d526$
//

#ifndef CEF_INCLUDE_CAPI_CEF_TASK_CAPI_H_
#define CEF_INCLUDE_CAPI_CEF_TASK_CAPI_H_
#pragma once

#include "include/capi/cef_base_capi.h"

#ifdef __cplusplus
extern "C" {
#endif

///
// Implement this structure for asynchronous task execution. If the task is
// posted successfully and if the associated message loop is still running then
// the execute() function will be called on the target thread. If the task fails
// to post then the task object may be destroyed on the source thread instead of
// the target thread. For this reason be cautious when performing work in the
// task object destructor.
///
typedef struct _cef_task_t {
  ///
  // Base structure.
  ///
  cef_base_ref_counted_t base;

  ///
  // Method that will be executed on the target thread.
  ///
  void(CEF_CALLBACK* execute)(struct _cef_task_t* self);
} cef_task_t;

///
// Structure that asynchronously executes tasks on the associated thread. It is
// safe to call the functions of this structure on any thread.
//
// CEF maintains multiple internal threads that are used for handling different
// types of tasks in different processes. The cef_thread_id_t definitions in
// cef_types.h list the common CEF threads. Task runners are also available for
// other CEF threads as appropriate (for example, V8 WebWorker threads).
///
typedef struct _cef_task_runner_t {
  ///
  // Base structure.
  ///
  cef_base_ref_counted_t base;

  ///
  // Returns true (1) if this object is pointing to the same task runner as
  // |that| object.
  ///
  int(CEF_CALLBACK* is_same)(struct _cef_task_runner_t* self,
                             struct _cef_task_runner_t* that);

  ///
  // Returns true (1) if this task runner belongs to the current thread.
  ///
  int(CEF_CALLBACK* belongs_to_current_thread)(struct _cef_task_runner_t* self);

  ///
  // Returns true (1) if this task runner is for the specified CEF thread.
  ///
  int(CEF_CALLBACK* belongs_to_thread)(struct _cef_task_runner_t* self,
                                       cef_thread_id_t threadId);

  ///
  // Post a task for execution on the thread associated with this task runner.
  // Execution will occur asynchronously.
  ///
  int(CEF_CALLBACK* post_task)(struct _cef_task_runner_t* self,
                               struct _cef_task_t* task);

  ///
  // Post a task for delayed execution on the thread associated with this task
  // runner. Execution will occur asynchronously. Delayed tasks are not
  // supported on V8 WebWorker threads and will be executed without the
  // specified delay.
  ///
  int(CEF_CALLBACK* post_delayed_task)(struct _cef_task_runner_t* self,
                                       struct _cef_task_t* task,
                                       int64 delay_ms);
} cef_task_runner_t;

///
// Returns the task runner for the current thread. Only CEF threads will have
// task runners. An NULL reference will be returned if this function is called
// on an invalid thread.
///
CEF_EXPORT cef_task_runner_t* cef_task_runner_get_for_current_thread();

///
// Returns the task runner for the specified CEF thread.
///
CEF_EXPORT cef_task_runner_t* cef_task_runner_get_for_thread(
    cef_thread_id_t threadId);

///
// Returns true (1) if called on the specified thread. Equivalent to using
// cef_task_tRunner::GetForThread(threadId)->belongs_to_current_thread().
///
CEF_EXPORT int cef_currently_on(cef_thread_id_t threadId);

///
// Post a task for execution on the specified thread. Equivalent to using
// cef_task_tRunner::GetForThread(threadId)->PostTask(task).
///
CEF_EXPORT int cef_post_task(cef_thread_id_t threadId, cef_task_t* task);

///
// Post a task for delayed execution on the specified thread. Equivalent to
// using cef_task_tRunner::GetForThread(threadId)->PostDelayedTask(task,
// delay_ms).
///
CEF_EXPORT int cef_post_delayed_task(cef_thread_id_t threadId,
                                     cef_task_t* task,
                                     int64 delay_ms);

#ifdef __cplusplus
}
#endif

#endif  // CEF_INCLUDE_CAPI_CEF_TASK_CAPI_H_
