import QtQuick
import QtQml.Models

DelegateModel {
    id: root
    model: null

    // --- 1. Існуючі властивості ---
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

            // --- БЛОК ФІЛЬТРАЦІЇ ---

            // 1. Стандартні фільтри (Папки, статуси)
            if (filterStatus !== -1 && data.status !== filterStatus) isVisible = false;
            if (filterFolder !== -1 && data.id_fol !== filterFolder) isVisible = false;
            if (starredOnly && !data.isStarred) isVisible = false;
            if (savedOnly && !data.isSaved) isVisible = false;

            // 2. Фільтри прочитаних/непрочитаних
            if (seenOnly && !data.isSeen) isVisible = false;
            if (unseenOnly && data.isSeen) isVisible = false;

            // 3. Фільтр вкладень (Припускаємо, що C++ бекенд або mock-модель матиме поле hasAttachments)
            // Якщо зараз у mock-моделі його немає, ця умова просто не пропустить листи, коли чекбокс активний
            if (attachmentsOnly && !data.hasAttachments) isVisible = false;

            // 4. Фільтрація по датах
            // Оскільки зараз дати — це текстові рядки ("10:45 AM", "Yesterday"), їх неможливо точно порівняти математично.
            // Щойно C++ бекенд почне віддавати реальний час (наприклад, у мілісекундах 'data.timestamp'), розкоментуй цю логіку:
            /*
            if (dateFilter !== "all" && data.timestamp) {
                let now = new Date().getTime();
                let msgDate = data.timestamp;

                // 86400000 - це кількість мілісекунд у дні
                if (dateFilter === "today" && (now - msgDate > 86400000)) isVisible = false;
                if (dateFilter === "week" && (now - msgDate > 604800000)) isVisible = false;
                if (dateFilter === "month" && (now - msgDate > 2592000000)) isVisible = false;
                if (dateFilter === "year" && (now - msgDate > 31536000000)) isVisible = false;
            }
            */

            // 5. Текстовий пошук (найважча операція, тому робимо її в кінці, якщо лист ще не відсіяно)
            if (isVisible && query !== "") {
                var subj = data.subject ? data.subject.toLowerCase() : "";
                var txt = data.body ? data.body.toLowerCase() : "";
                var rec = data.receiver ? data.receiver.toLowerCase() : "";

                if (!subj.includes(query) && !txt.includes(query) && !rec.includes(query)) {
                    isVisible = false;
                }
            }

            // Якщо після всіх перевірок лист підходить — показуємо його
            if (isVisible) {
                items.get(i).inVisible = true;
            }
        }
    }

    // Скидання абсолютно всіх фільтрів
    function resetFilters() {
        filterStatus = -1;
        filterFolder = -1;
        starredOnly = false;
        savedOnly = false;
        searchQuery = "";

        // Скидання нових фільтрів
        seenOnly = false;
        unseenOnly = false;
        attachmentsOnly = false;
        dateFilter = "all";
        dateFrom = "";
        dateTo = "";
    }

    // --- Слухачі подій (Тригери) ---
    onFilterStatusChanged: refilter()
    onFilterFolderChanged: refilter()
    onStarredOnlyChanged: refilter()
    onSavedOnlyChanged: refilter()
    onSearchQueryChanged: refilter()

    // Нові слухачі
    onSeenOnlyChanged: refilter()
    onUnseenOnlyChanged: refilter()
    onAttachmentsOnlyChanged: refilter()
    onDateFilterChanged: refilter()
    onDateFromChanged: refilter()
    onDateToChanged: refilter()

    Component.onCompleted: refilter()
}
