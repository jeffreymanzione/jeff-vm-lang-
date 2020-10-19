// context.c
//
// Created on: Jun 13, 2020
//     Author: Jeff Manzione

#include "vm/process/context.h"

#include "entity/class/classes.h"
#include "entity/function/function.h"
#include "entity/module/modules.h"
#include "entity/object.h"
#include "program/tape.h"
#include "vm/intern.h"
#include "vm/process/processes.h"

Heap *_context_heap(Context *ctx);

void context_init(Context *ctx, Object *self, Object *member_obj,
                  Module *module, uint32_t instruction_pos) {
  ctx->self = entity_object(self);
  ctx->member_obj = member_obj;
  ctx->module = module;
  ctx->tape = module->_tape;
  ctx->ins = instruction_pos;
  ctx->func = NULL;
  ctx->error = NULL;
  ctx->catch_ins = -1;
}

void context_finalize(Context *ctx) { ASSERT(NOT_NULL(ctx)); }

inline const Instruction *context_ins(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return tape_get(ctx->tape, ctx->ins);
}

inline Heap *_context_heap(Context *ctx) {
  return ctx->parent_task->parent_process->heap;
}

inline Object *context_self(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return ctx->self.obj;
}

inline Module *context_module(Context *ctx) {
  ASSERT(NOT_NULL(ctx));
  return ctx->module;
}

Entity *context_lookup(Context *ctx, const char id[]) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id));
  if (SELF == id) {
    return &ctx->self;
  }
  Entity *member = object_get(ctx->member_obj, id);
  if (NULL != member) {
    return member;
  }
  Task *task = ctx->parent_task;
  Context *parent_context = ctx->previous_context;
  while (NULL != parent_context && NULL == (member = object_get(parent_context->member_obj, id))) {
    parent_context = parent_context->previous_context;
  }
  if (NULL != member) {
    return member;
  }
  member = object_get(ctx->self.obj, id);
  if (NULL != member) {
    return member;
  }

  const Function *f = class_get_function(ctx->self.obj->_class, id);
  if (NULL != f) {
    Object *fn_ref = heap_new(task->parent_process->heap, Class_FunctionRef);
    __function_ref_init(fn_ref, ctx->self.obj, f, f->_is_anon ? ctx : NULL);
    return object_set_member_obj(task->parent_process->heap, ctx->self.obj, id,
                                 fn_ref);
  }

  member = object_get(ctx->module->_reflection, id);
  if (NULL != member) {
    return member;
  }

  Object *obj = module_lookup(ctx->module, id);
  if (NULL != obj) {
    return object_set_member_obj(_context_heap(ctx), ctx->module->_reflection,
                                 id, obj);
  }

  member = object_get(Module_builtin->_reflection, id);
  if (NULL != member) {
    return member;
  }

  obj = module_lookup(Module_builtin, id);
  if (NULL != obj) {
    return object_set_member_obj(_context_heap(ctx),
                                 Module_builtin->_reflection, id, obj);
  }
  return NULL;
}

void context_let(Context *ctx, const char id[], const Entity *e) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id), NOT_NULL(e));
  object_set_member(_context_heap(ctx), ctx->member_obj, id, e);
}

void context_set(Context *ctx, const char id[], const Entity *e) {
  ASSERT(NOT_NULL(ctx), NOT_NULL(id), NOT_NULL(e));
  Entity *member = NULL;
  if (NULL != object_get(ctx->member_obj, id)) {
    object_set_member(_context_heap(ctx), ctx->member_obj, id, e);
    return;
  }
  Context *parent_context = ctx->previous_context;
  while (NULL != parent_context && NULL == (member = object_get(parent_context->member_obj, id))) {
    parent_context = parent_context->previous_context;
  }
  if (NULL != member) {
    object_set_member(_context_heap(parent_context),
                      parent_context->member_obj, id, e);
    return;
  }
  if (NULL != object_get(ctx->self.obj, id)) {
    object_set_member(_context_heap(ctx), ctx->self.obj, id, e);
    return;
  }
  object_set_member(_context_heap(ctx), ctx->member_obj, id, e);
}

inline void context_set_function(Context *ctx, const Function *func) {
  ctx->func = func;
}