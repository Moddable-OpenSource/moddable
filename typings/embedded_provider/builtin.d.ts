import type { Digital } from 'embedded:io/digital'
declare module 'embedded:provider/builtin' {
  const device: {
    io?: {
      Digital?: Digital;
    }
  };
  export default device;
}
