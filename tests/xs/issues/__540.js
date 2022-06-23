/*---
description: https://github.com/Moddable-OpenSource/moddable/issues/540
flags: [onlyStrict]
---*/

function func(descriptors) {
  const obj = {}
  
  for (const [key, descriptor] of Object.entries(descriptors)) {
    const type =
      typeof descriptor.type == 'string'
        ? descriptor.type.toLowerCase()
      : typeof descriptor.type.name == 'string'
        ? descriptor.type.name.toLowerCase()
      : ''
    if ((type == 'digital')) {
      if (typeof descriptor.mode == 'string') {
        // It should be possible to declare using const, but the let to be declared next will be treated as const.
        const mode = descriptor.mode.toLowerCase()
        if (mode == 'output') descriptor.mode = 1
      }
      if (descriptor.mode == 1) {
        let value = descriptor.default || false
        //value = !value  // ERROR!! set value: const!
        
        Object.defineProperty(obj, key, {
          get() {
            return value
          },
          set(val) {
            value = val // ERROR!! set value: const!
          },
          enumerable: true,
        })
      }
    }
  }
  
  return obj
}

const obj = func({
  prop: {
    type: 'Digital',
    mode: 'Output',
  }
})

obj.prop = true
