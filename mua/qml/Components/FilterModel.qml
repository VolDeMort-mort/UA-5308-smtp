import QtQuick
import QtQml.Models

DelegateModel {
    id: root
    model: null

    property int filterStatus: -1
    property int filterFolder: -1
    property bool starredOnly: false
    property bool savedOnly: false
    property string searchQuery: ""

    property bool seenOnly: false
    property bool unseenOnly: false
    property bool attachmentsOnly: false
    property string dateFilter: "all" // "all", "today", "week", "month", "year"
    property string dateFrom: ""
    property string dateTo: ""

    filterOnGroup: "visible"
    groups: [
        DelegateModelGroup { id: visibleItems; name: "visible" }
    ]

    function refilter() {
        if (!items || items.count === 0) return;

        items.removeGroups(0, items.count, ["visible"]);

        var query = searchQuery.toLowerCase().trim();

        for (var i = 0; i < items.count; i++) {
            var data = items.get(i).model;
            var isVisible = true;

            if (filterStatus !== -1 && data.status !== filterStatus) isVisible = false;
            if (filterFolder !== -1 && data.id_fol !== filterFolder) isVisible = false;
            if (starredOnly && !data.isStarred) isVisible = false;
            if (savedOnly && !data.isSaved) isVisible = false;

            
            if (seenOnly && !data.isSeen) isVisible = false;
            if (unseenOnly && data.isSeen) isVisible = false;

            if (attachmentsOnly && !data.hasAttachments) isVisible = false;

            if (dateFilter !== "all" && data.timestamp) {
                let now = new Date().getTime();
                let msgDate = data.timestamp;

                if (dateFilter === "today" && (now - msgDate > 86400000)) isVisible = false;
                if (dateFilter === "week" && (now - msgDate > 604800000)) isVisible = false;
                if (dateFilter === "month" && (now - msgDate > 2592000000)) isVisible = false;
                if (dateFilter === "year" && (now - msgDate > 31536000000)) isVisible = false;
            }
         
            if (isVisible && query !== "") {
                var subj = data.subject ? data.subject.toLowerCase() : "";
                var txt = data.body ? data.body.toLowerCase() : "";
                var rec = data.receiver ? data.receiver.toLowerCase() : "";

                if (!subj.includes(query) && !txt.includes(query) && !rec.includes(query)) {
                    isVisible = false;
                }
            }

            if (isVisible) {
                items.get(i).inVisible = true;
            }
        }
    }

    function resetFilters() {
        filterStatus = -1;
        filterFolder = -1;
        starredOnly = false;
        savedOnly = false;
        searchQuery = "";

    
        seenOnly = false;
        unseenOnly = false;
        attachmentsOnly = false;
        dateFilter = "all";
        dateFrom = "";
        dateTo = "";
    }


    onFilterStatusChanged: refilter()
    onFilterFolderChanged: refilter()
    onStarredOnlyChanged: refilter()
    onSavedOnlyChanged: refilter()
    onSearchQueryChanged: refilter()

   
    onSeenOnlyChanged: refilter()
    onUnseenOnlyChanged: refilter()
    onAttachmentsOnlyChanged: refilter()
    onDateFilterChanged: refilter()
    onDateFromChanged: refilter()
    onDateToChanged: refilter()

    Component.onCompleted: refilter()
}
