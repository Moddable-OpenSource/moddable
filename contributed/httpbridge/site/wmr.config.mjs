import zip from 'rollup-plugin-zip';

export function build({ plugins }) {
  plugins.push(
    zip({file: 'site.zip'})
  );
}
