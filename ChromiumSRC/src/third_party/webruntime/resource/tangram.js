apppage.sendMessage("NEW_TAB_PAGE_LOADED", "", "", "", "", "");

apppage.onMessageReceived = function (e) {
    if (e.eventId == "MESSAGE") {
        var channel = e.param2.substr(0, e.param2.indexOf('@'));
        if (channel == "system" && e.param5 == "__getElementById__") {
            var id = e.param4;
            var htmlElement = document.getElementById(id);
            var htmlInnerHTML = htmlElement.innerHTML;
            apppage.sendMessage("system@" + e.param1, htmlInnerHTML, "__getElementById__", e.param3);
        }
    }
}