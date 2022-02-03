//import htmlMinifier from 'rollup-plugin-html-minifier';
import zip from 'rollup-plugin-zip';
//import embedCSS from 'rollup-plugin-embed-css';

export function build({ plugins }) {
  plugins.push(
    //embedCSS({/* Options */}),
    //htmlMinifier({
      // any options here
    //}),
    zip({file: 'site.zip'})
  );
}
