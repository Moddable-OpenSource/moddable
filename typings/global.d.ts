import System_ from "embedded:io/system"
import Device from "embedded:provider/builtin"

export {}
declare global {
  // NOTE: `System` is non-standard and temporary to support the IO examples. Breaking changes are possible.
  var System: typeof System_;
  var device: typeof Device;

  interface Math {
    idiv(a: number, b: number): number
    imod(a: number, b: number): number // (x%y+y)%y (always pos results)
    irem(a: number, b: number): number // x%y (neg x -> neg result)
    idivmod(a: number, b: number): [number, number] // [x/y, (x%y+y)%y]
    imuldiv(a: number, b: number, c: number): number // (a * b) / c
    mod(a: number, b: number): number // (x%y+y)%y (floating point)
  }

}
