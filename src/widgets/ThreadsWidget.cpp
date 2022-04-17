#include <QShortcut>
#include "ThreadsWidget.h"
#include "ui_ThreadsWidget.h"
#include "common/JsonModel.h"
#include "QuickFilterView.h"
#include <rz_debug.h>

#include "core/MainWindow.h"

#define DEBUGGED_PID (-1)

enum ColumnIndex {
    COLUMN_PID = 0,
    COLUMN_STATUS,
    COLUMN_PATH,
};

ThreadsWidget::ThreadsWidget(MainWindow *main) : CutterDockWidget(main), ui(new Ui::ThreadsWidget)
{
    ui->setupUi(this);

    // Setup threads model
    modelThreads = new QStandardItemModel(1, 3, this);
    modelThreads->setHorizontalHeaderItem(COLUMN_PID, new QStandardItem(tr("PID")));
    modelThreads->setHorizontalHeaderItem(COLUMN_STATUS, new QStandardItem(tr("Status")));
    modelThreads->setHorizontalHeaderItem(COLUMN_PATH, new QStandardItem(tr("Path")));
    ui->viewThreads->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->viewThreads->verticalHeader()->setVisible(false);
    ui->viewThreads->setFont(Config()->getFont());

    modelFilter = new ThreadsFilterModel(this);
    modelFilter->setSourceModel(modelThreads);
    ui->viewThreads->setModel(modelFilter);

    // CTRL+F switches to the filter view and opens it in case it's hidden
    QShortcut *searchShortcut = new QShortcut(QKeySequence::Find, this);
    connect(searchShortcut, &QShortcut::activated, ui->quickFilterView,
            &QuickFilterView::showFilter);
    searchShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    // ESC switches back to the thread table and clears the buffer
    QShortcut *clearShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this);
    connect(clearShortcut, &QShortcut::activated, this, [this]() {
        ui->quickFilterView->clearFilter();
        ui->viewThreads->setFocus();
    });
    clearShortcut->setContext(Qt::WidgetWithChildrenShortcut);

    refreshDeferrer = createRefreshDeferrer([this]() { updateContents(); });

    connect(ui->quickFilterView, &QuickFilterView::filterTextChanged, modelFilter,
            &ThreadsFilterModel::setFilterWildcard);
    connect(Core(), &CutterCore::refreshAll, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::registersChanged, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::debugTaskStateChanged, this, &ThreadsWidget::updateContents);
    // Seek doesn't necessarily change when switching threads/processes
    connect(Core(), &CutterCore::switchedThread, this, &ThreadsWidget::updateContents);
    connect(Core(), &CutterCore::switchedProcess, this, &ThreadsWidget::updateContents);
    connect(Config(), &Configuration::fontsUpdated, this, &ThreadsWidget::fontsUpdatedSlot);
    connect(ui->viewThreads, &QTableView::activated, this, &ThreadsWidget::onActivated);
}

ThreadsWidget::~ThreadsWidget() {}

void ThreadsWidget::updateContents()
{
    if (!refreshDeferrer->attemptRefresh(nullptr)) {
        return;
    }

    if (!Core()->currentlyDebugging) {
        // Remove rows from the previous debugging session
        modelThreads->removeRows(0, modelThreads->rowCount());
        return;
    }

    if (Core()->isDebugTaskInProgress()) {
        ui->viewThreads->setDisabled(true);
    } else {
        setThreadsGrid();
        ui->viewThreads->setDisabled(false);
    }
}

QString ThreadsWidget::translateStatus(const char status)
{
    switch (status) {
    case RZ_DBG_PROC_STOP:
        return "Stopped";
    case RZ_DBG_PROC_RUN:
        return "Running";
    case RZ_DBG_PROC_SLEEP:
        return "Sleeping";
    case RZ_DBG_PROC_ZOMBIE:
        return "Zombie";
    case RZ_DBG_PROC_DEAD:
        return "Dead";
    case RZ_DBG_PROC_RAISED:
        return "Raised event";
    default:
        return "Unknown status";
    }
}

void ThreadsWidget::setThreadsGrid()
{
    int i = 0;
    QFont font;

    for (const auto &threadsItem : Core()->getProcessThreads(DEBUGGED_PID)) {
        st64 pid = threadsItem.pid;
        QString status = translateStatus(threadsItem.status);
        QString path = threadsItem.path;
        bool current = threadsItem.current;
        // Use bold font to highlight active thread
        font.setBold(current);
        QStandardItem *rowPid = new QStandardItem(QString::number(pid));
        rowPid->setFont(font);
        QStandardItem *rowStatus = new QStandardItem(status);
        rowStatus->setFont(font);
        QStandardItem *rowPath = new QStandardItem(path);
        rowPath->setFont(font);
        modelThreads->setItem(i, COLUMN_PID, rowPid);
        modelThreads->setItem(i, COLUMN_STATUS, rowStatus);
        modelThreads->setItem(i, COLUMN_PATH, rowPath);
        i++;
    }

    // Remove irrelevant old rows
    if (modelThreads->rowCount() > i) {
        modelThreads->removeRows(i, modelThreads->rowCount() - i);
    }

    modelFilter->setSourceModel(modelThreads);
    ui->viewThreads->resizeColumnsToContents();
    ;
}

void ThreadsWidget::fontsUpdatedSlot()
{
    ui->viewThreads->setFont(Config()->getFont());
}

void ThreadsWidget::onActivated(const QModelIndex &index)
{
    if (!index.isValid())
        return;

    int tid = modelFilter->data(index.sibling(index.row(), COLUMN_PID)).toInt();

    // Verify that the selected tid is still in the threads list since dpt= will
    // attach to any given id. If it isn't found simply update the UI.
    for (const auto &value : Core()->getProcessThreads(DEBUGGED_PID)) {
        if (tid == value.pid) {
            Core()->setCurrentDebugThread(tid);
            break;
        }
    }

    updateContents();
}

ThreadsFilterModel::ThreadsFilterModel(QObject *parent) : QSortFilterProxyModel(parent)
{
    setFilterCaseSensitivity(Qt::CaseInsensitive);
    setSortCaseSensitivity(Qt::CaseInsensitive);
}

bool ThreadsFilterModel::filterAcceptsRow(int row, const QModelIndex &parent) const
{
    // All columns are checked for a match
    for (int i = COLUMN_PID; i <= COLUMN_PATH; ++i) {
        QModelIndex index = sourceModel()->index(row, i, parent);
        if (qhelpers::filterStringContains(sourceModel()->data(index).toString(), this)) {
            return true;
        }
    }

    return false;
}
