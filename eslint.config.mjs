import js from "@eslint/js";
// import globals from "globals";
import tseslint from "typescript-eslint";
import {
	defineConfig
} from "eslint/config";

export default defineConfig([{
		files: ["**/*.{js,ts}"],
		plugins: {
			js
		},
		extends: ["js/recommended"],
		languageOptions: {
			ecmaVersion: 2025,
			sourceType: "module",
			globals: {
				trace: "readonly",
				Compartment: "readonly",

				Native: "readonly",
				native: "readonly",

				Host: "readonly",
				screen: "readonly",
				backlight: "readonly",

				device: "readonly",
				System: "readonly",

				self: "readonly",

				Application: "readonly",
				Behavior: "readonly",
				CLUT: "readonly",
				Column: "readonly",
				Content: "readonly",
				Container: "readonly",
				DeferLink: "readonly",
				Die: "readonly",
				Label: "readonly",
				Layout: "readonly",
				Link: "readonly",
				Locals: "readonly",
				Port: "readonly",
				Resource: "readonly",
				Row: "readonly",
				Scroller: "readonly",
				Skin: "readonly",
				Style: "readonly",
				Template: "readonly",
				Text: "readonly",
				Texture: "readonly",
				TouchLink: "readonly",
				Transition: "readonly",

				application: "readonly",
				blendColors: "readonly",
				hsl: "readonly",
				hsla: "readonly",
				rgb: "readonly",
				rgba: "readonly",
				template: "readonly",

				Image: "readonly",
				ImageBuffer: "readonly",
				Outline: "readonly",
				QRCode: "readonly",
				Shape: "readonly",
			}
		}
	},
	tseslint.configs.recommended,
	{
		rules: {
			"no-debugger": "warn",
			"no-constant-condition": "warn",
			"@typescript-eslint/no-unused-vars": ["warn", {
				"varsIgnorePattern": "^_", 
			}],
			"@typescript-eslint/no-this-alias": "warn",
			"prefer-const": "off",
			"no-restricted-globals": [
				"warn",
				{
					name: "System",
					message: "System global is deprecated and will be removed"
				},
				{
					name: "Host",
					message: "Host global is deprecated. Functionality migrating to ECMA-419 APIs"
				},
			],
		}
	}
]);
