// error.c
//
// Created on: Sept 15, 2020
//     Author: Jeff Manzione

#include "entity/native/error.h"

#include "alloc/arena/intern.h"
#include "entity/class/classes.h"
#include "entity/entity.h"
#include "entity/native/native.h"
#include "entity/object.h"
#include "entity/string/string_helper.h"
#include "entity/tuple/tuple.h"
#include "heap/heap.h"
#include "vm/intern.h"
#include "vm/process/processes.h"
#include "vm/process/task.h"

static Class *Class_StackLine;

typedef struct {
  Module *module;
  Function *func;
  Token *error_token;
} _StackLine;

// typedef struct {
//   TokenType type;
//   int col, line;
//   size_t len;
//   const char *text;
// } Token;

void _error_init(Object *obj) {}
void _error_delete(Object *obj) {}

void _stackline_init(Object *obj) { obj->_internal_obj = ALLOC2(_StackLine); }
void _stackline_delete(Object *obj) { DEALLOC(obj->_internal_obj); }

Entity _stackline_module(Task *task, Context *ctx, Object *obj, Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  return entity_object(sl->module->_reflection);
}

Entity _stackline_function(Task *task, Context *ctx, Object *obj,
                           Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  if (NULL == sl->func) {
    return NONE_ENTITY;
  }
  return entity_object(sl->func->_reflection);
}

Entity _stackline_token(Task *task, Context *ctx, Object *obj, Entity *args) {
  _StackLine *sl = (_StackLine *)obj->_internal_obj;
  Object *tuple_obj = heap_new(task->parent_process->heap, Class_Tuple);
  tuple_obj->_internal_obj = tuple_create(3);
  Entity token_text = entity_object(string_new(
      task->parent_process->heap, sl->error_token->text, sl->error_token->len));
  tuple_set(task->parent_process->heap, tuple_obj, 0, &token_text);
  Entity line = entity_int(sl->error_token->line);
  tuple_set(task->parent_process->heap, tuple_obj, 1, &line);
  Entity col = entity_int(sl->error_token->col);
  tuple_set(task->parent_process->heap, tuple_obj, 2, &col);
  return entity_object(tuple_obj);
}

Entity _error_constructor(Task *task, Context *ctx, Object *obj, Entity *args) {
  if (NULL == args || OBJECT != args->type ||
      Class_String != args->obj->_class) {
    ERROR("Error argument is not a String.");
  }
  object_set_member_obj(task->parent_process->heap, obj, intern("message"),
                        args->obj);

  Object *stacktrace = heap_new(task->parent_process->heap, Class_Array);
  Task *t = task;
  // printf("context_count=%d tok=\n", task_context_count(task), ctx->);
  while (NULL != t) {
    for (int i = task_context_count(task) - 1; i >= 0; --i) {
      Object *stackline = heap_new(task->parent_process->heap, Class_StackLine);
      _StackLine *sl = (_StackLine *)stackline->_internal_obj;
      Context *c = task_get_context_for_index(t, i);
      sl->module = c->module;
      sl->func = (Function *)c->func; // blessed
      sl->error_token =
          (Token *)tape_get_source(c->tape, c->ins)->token; // blessed
      Entity sl_e = entity_object(stackline);
      array_add(task->parent_process->heap, stacktrace, &sl_e);
    }
    t = t->dependent_task;
  }
  object_set_member_obj(task->parent_process->heap, obj, intern("stacktrace"),
                        stacktrace);
  return entity_object(obj);
}

Object *error_new(Task *task, Context *ctx, Object *error_msg) {
  Entity error_msg_e = entity_object(error_msg);
  Object *err = heap_new(task->parent_process->heap, Class_Error);
  _error_constructor(task, ctx, err, &error_msg_e);
  return err;
}

void error_add_native(Module *error) {
  Class_Error = native_class(error, ERROR_NAME, _error_init, _error_delete);
  native_method(Class_Error, CONSTRUCTOR_KEY, _error_constructor);

  Class_StackLine =
      native_class(error, STACKLINE_NAME, _stackline_init, _stackline_delete);
  native_method(Class_StackLine, MODULE_KEY, _stackline_module);
  native_method(Class_StackLine, intern("function"), _stackline_function);
  native_method(Class_StackLine, intern("__token"), _stackline_token);
}
