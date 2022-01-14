package main

import (
    "C"
	"golang.org/x/text/language"
	"golang.org/x/text/language/display"
)

//export DisplayLanguage
func DisplayLanguage(lang_c *C.char) *C.char {
	var lang string
	var ret string
	lang = C.GoString(lang_c)
	switch (lang) {
	case "bh":
		ret = "Bihari";
		break;
	case "bn":
		ret = "Bengali";
		break;
	case "ckb":
		ret = "Kurdish";
		break;
	case "dra":
		ret = "Dravidian";
		break;
	case "en-GB":
		ret = "U.K. English";
		break;
	case "en-US":
		ret = "U.S. English";
		break;
	case "eo-xsistemo":
		ret = "Esperanto x-system";;
		break;
	case "gez":
		ret = "Ethiopic";
		break;
	case "mun":
		ret = "Munda";
		break;
	case "no":
		ret = "Norwegian";
		break;
	case "or":
		ret = "Oriya";
		break;
	default:
		var namer display.Namer
		namer = display.English.Languages()
		ret = namer.Name(language.MustParse(lang))
	}
	return C.CString(ret)
}

//export NativeLanguage
func NativeLanguage(lang_c *C.char) *C.char {
	var lang string
	var ret string
	var namer display.Namer
	lang = C.GoString(lang_c)
	namer = display.Self
	ret = namer.Name(language.MustParse(lang))
	return C.CString(ret)
}

// main function required for interfacing with C
func main() {}
