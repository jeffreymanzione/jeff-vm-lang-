module builtin

def str(input) {
  if ~input return 'None'
  if input is Object return input.to_s()
  __stringify(input)  ; Native C function
}

def cat(args) {
  result = ''
  if ~(args is Tuple) {
    result.extend(str(args))
    return result
  }
  for i=0, i < args.len(), i=i+1 {
    result.extend(str(args[i]))
  }
  return result
}

def hash(x) {
  if x is Object {
    return x.hash()
  }
  return Int(x)
}

class Object {
  method to_s() {
    return cat(
        'Instance of ',
        self.class().module().name(),
        '.',
        self.class().name())
  }
}

class Class {
  method to_s() {
    return cat('class ', name())
  }
}

class String {
  method to_s() {
    return self
  }
  method join(strs) {
    result = ''
    if strs.len() == 0 {
      return result
    }
    result.extend(str(strs[0]))
    for i=1, i<strs.len(), i=i+1 {
      result.extend(self)
      result.extend(str(strs[i]))
    }
    return result
  }
}

class Tuple {
  method ==(other) {
    if (len() != other.len()) return False
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return False
      }
    }
    return True
  }
  method !=(other) {
    if (len() != other.len()) return True
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return True
      }
    }
    return False
  }
  method to_s() {
    result = '('
    result.extend(','.join(self))
    result.extend(')')
    return result
  }
}

class Array {
  method ==(other) {
    if (len() != other.len()) return False
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return False
      }
    }
    return True
  }
  method !=(other) {
    if (len() != other.len()) return True
    for i=0, i<len(), i=i+1 {
      if (self[i] != other[i]) {
        return True
      }
    }
    return False
  }
  method to_s() {
    result = '['
    result.extend(','.join(self))
    result.extend(']')
    return result
  }
  method each(fn) {
    for i=0, i<len(), i=i+1 {
      fn(self[i])
    }
  }
  method map(fn) {
    result = []
    result[len()-1] = None
    for i=0, i<len(), i=i+1 {
      result[i] = fn(self[i])
    } 
    return result
  }
}

class Function {
  method to_s() {
    if self.is_method() {
      return cat('Method(', self.module().name(), '.', self.parent_class().name(), '.', self.name(), ')')
    } else {
      return cat('Function(', self.module().name(), '.', self.name(), ')')
    }
  }
}

class FunctionRef {
  method to_s() {
    cat('FunctionRef(obj=', self.obj(), ',func=', self.func(), ')')
  }
}

class Iterator {
  new(field has_next, field next) {}
}

class Range {
  method to_s() {
    cat(start(), ':', inc(), ':', end())
  }
}

def range(params) {
  start = params[0]
  if params.len() == 3 {
    inc = params[1]
    end = params[2]
  } else {
    inc = 1
    end = params[1]
  }
  Range(start, inc, end)
}