import QtQuick
import QtQml.Models

DelegateModel {
    id: root
    model: null

    // Стан фільтрації
    property int filterStatus: -1
    property int filterFolder: -1
    property bool starredOnly: false
    property bool savedOnly: false

    filterOnGroup: "visible"
    groups: [
        DelegateModelGroup { id: visibleItems; name: "visible" }
    ]

    function refilter() {
        if (!items || items.count === 0) return;

        items.removeGroups(0, items.count, ["visible"]);

        for (var i = 0; i < items.count; i++) {
            var data = items.get(i).model;
            var isVisible = true;

            if (filterStatus !== -1 && data.status !== filterStatus) isVisible = false;
            if (filterFolder !== -1 && data.id_fol !== filterFolder) isVisible = false;
            if (starredOnly && !data.isStarred) isVisible = false;
            if (savedOnly && !data.isSaved) isVisible = false;

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
    }


    onFilterStatusChanged: refilter()
    onFilterFolderChanged: refilter()
    onStarredOnlyChanged: refilter()
    onSavedOnlyChanged: refilter()

    Component.onCompleted: refilter()
}
