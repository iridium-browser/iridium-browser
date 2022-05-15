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
	case "kmr":
		ret = "Northern Kurdish";
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
	case "sah":
		ret = "Yakut"; // not "Sakha"
		break;
	case "st":
		ret = "Sesotho"; // South Africans say "Sesotho", not "Sotho" or "Southern Sotho"
		break;
	case "tn":
		ret = "Setswana"; // South Africans say "Setswana", not "Tswana"
		break;
	case "xh":
		ret = "isiXhosa"; // South Africans say "isiXhosa", not "Xhosa"
		break;
	case "ve":
		ret = "Tshivenda"; // South Africans say "Tshivenda", not "Venda"
		break;
	case "zu":
		ret = "isiZulu"; // South Africans say "isiZulu", not "Zulu"
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

//export DisplayRegion
func DisplayRegion(region_c *C.char) *C.char {
	var region string
	var ret string
	region = C.GoString(region_c)
	switch (region) {
	case "CA":
		ret = "Canada";
		break;
	case "GB":
		ret = "the U.K.";
		break;
	case "US":
		ret = "the U.S.";
		break;
	default:
		ret = "";
		break;
	}
	return C.CString(ret)
}

// main function required for interfacing with C
func main() {}
