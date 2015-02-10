// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

/**
 * Namespace for utility functions.
 */
var filelist = {};

/**
 * Custom column model for advanced auto-resizing.
 *
 * @param {!Array.<cr.ui.table.TableColumn>} tableColumns Table columns.
 * @extends {cr.ui.table.TableColumnModel}
 * @constructor
 */
function FileTableColumnModel(tableColumns) {
  cr.ui.table.TableColumnModel.call(this, tableColumns);
}

/**
 * The columns whose index is less than the constant are resizable.
 * @const
 * @type {number}
 * @private
 */
FileTableColumnModel.RESIZABLE_LENGTH_ = 4;

/**
 * Inherits from cr.ui.TableColumnModel.
 */
FileTableColumnModel.prototype.__proto__ =
    cr.ui.table.TableColumnModel.prototype;

/**
 * Minimum width of column.
 * @const
 * @type {number}
 * @private
 */
FileTableColumnModel.MIN_WIDTH_ = 10;

/**
 * Sets column width so that the column dividers move to the specified position.
 * This function also check the width of each column and keep the width larger
 * than MIN_WIDTH_.
 *
 * @private
 * @param {Array.<number>} newPos Positions of each column dividers.
 */
FileTableColumnModel.prototype.applyColumnPositions_ = function(newPos) {
  // Check the minimum width and adjust the positions.
  for (var i = 0; i < newPos.length - 2; i++) {
    if (newPos[i + 1] - newPos[i] < FileTableColumnModel.MIN_WIDTH_) {
      newPos[i + 1] = newPos[i] + FileTableColumnModel.MIN_WIDTH_;
    }
  }
  for (var i = newPos.length - 1; i >= 2; i--) {
    if (newPos[i] - newPos[i - 1] < FileTableColumnModel.MIN_WIDTH_) {
      newPos[i - 1] = newPos[i] - FileTableColumnModel.MIN_WIDTH_;
    }
  }
  // Set the new width of columns
  for (var i = 0; i < FileTableColumnModel.RESIZABLE_LENGTH_; i++) {
    this.columns_[i].width = newPos[i + 1] - newPos[i];
  }
};

/**
 * Normalizes widths to make their sum 100% if possible. Uses the proportional
 * approach with some additional constraints.
 *
 * @param {number} contentWidth Target width.
 * @override
 */
FileTableColumnModel.prototype.normalizeWidths = function(contentWidth) {
  var totalWidth = 0;
  var fixedWidth = 0;
  // Some columns have fixed width.
  for (var i = 0; i < this.columns_.length; i++) {
    if (i < FileTableColumnModel.RESIZABLE_LENGTH_)
      totalWidth += this.columns_[i].width;
    else
      fixedWidth += this.columns_[i].width;
  }
  var newTotalWidth = Math.max(contentWidth - fixedWidth, 0);
  var positions = [0];
  var sum = 0;
  for (var i = 0; i < FileTableColumnModel.RESIZABLE_LENGTH_; i++) {
    var column = this.columns_[i];
    sum += column.width;
    // Faster alternative to Math.floor for non-negative numbers.
    positions[i + 1] = ~~(newTotalWidth * sum / totalWidth);
  }
  this.applyColumnPositions_(positions);
};

/**
 * Handles to the start of column resizing by splitters.
 */
FileTableColumnModel.prototype.handleSplitterDragStart = function() {
  this.initializeColumnPos();
};

/**
 * Handles to the end of column resizing by splitters.
 */
FileTableColumnModel.prototype.handleSplitterDragEnd = function() {
  this.destroyColumnPos();
};

/**
 * Initialize columnPos_ which is used in setWidthAndKeepTotal().
 */
FileTableColumnModel.prototype.initializeColumnPos = function() {
  this.columnPos_ = [0];
  for (var i = 0; i < this.columns_.length; i++) {
    this.columnPos_[i + 1] = this.columns_[i].width + this.columnPos_[i];
  }
};

/**
 * Destroy columnPos_ which is used in setWidthAndKeepTotal().
 */
FileTableColumnModel.prototype.destroyColumnPos = function() {
  this.columnPos_ = null;
};

/**
 * Sets the width of column with keeping the total width of table.
 * Before and after calling this method, you must initialize and destroy
 * columnPos with initializeColumnPos() and destroyColumnPos().
 * @param {number} columnIndex Index of column that is resized.
 * @param {number} columnWidth New width of the column.
 */
FileTableColumnModel.prototype.setWidthAndKeepTotal = function(
    columnIndex, columnWidth) {
  // Skip to resize 'selection' column
  if (columnIndex < 0 ||
      columnIndex >= FileTableColumnModel.RESIZABLE_LENGTH_ ||
      !this.columnPos_) {
    return;
  }

  // Calculate new positions of column splitters.
  var newPosStart =
      this.columnPos_[columnIndex] + Math.max(columnWidth,
                                              FileTableColumnModel.MIN_WIDTH_);
  var newPos = [];
  var posEnd = this.columnPos_[FileTableColumnModel.RESIZABLE_LENGTH_];
  for (var i = 0; i < columnIndex + 1; i++) {
    newPos[i] = this.columnPos_[i];
  }
  for (var i = columnIndex + 1;
       i < FileTableColumnModel.RESIZABLE_LENGTH_;
       i++) {
    var posStart = this.columnPos_[columnIndex + 1];
    newPos[i] = (posEnd - newPosStart) *
                (this.columnPos_[i] - posStart) /
                (posEnd - posStart) +
                newPosStart;
    // Faster alternative to Math.floor for non-negative numbers.
    newPos[i] = ~~newPos[i];
  }
  newPos[columnIndex] = this.columnPos_[columnIndex];
  newPos[FileTableColumnModel.RESIZABLE_LENGTH_] = posEnd;
  this.applyColumnPositions_(newPos);

  // Notifiy about resizing
  cr.dispatchSimpleEvent(this, 'resize');
};

/**
 * Obtains a column by the specified horizontal position.
 * @param {number} x Horizontal position.
 * @return {Object} The object that contains column index, column width, and
 *     hitPosition where the horizontal position is hit in the column.
 */
FileTableColumnModel.prototype.getHitColumn = function(x) {
  for (var i = 0; x >= this.columns_[i].width; i++) {
    x -= this.columns_[i].width;
  }
  if (i >= this.columns_.length)
    return null;
  return {index: i, hitPosition: x, width: this.columns_[i].width};
};

/**
 * Custom splitter that resizes column with retaining the sum of all the column
 * width.
 */
var FileTableSplitter = cr.ui.define('div');

/**
 * Inherits from cr.ui.TableSplitter.
 */
FileTableSplitter.prototype.__proto__ = cr.ui.TableSplitter.prototype;

/**
 * Handles the drag start event.
 */
FileTableSplitter.prototype.handleSplitterDragStart = function() {
  cr.ui.TableSplitter.prototype.handleSplitterDragStart.call(this);
  this.table_.columnModel.handleSplitterDragStart();
};

/**
 * Handles the drag move event.
 * @param {number} deltaX Horizontal mouse move offset.
 */
FileTableSplitter.prototype.handleSplitterDragMove = function(deltaX) {
  this.table_.columnModel.setWidthAndKeepTotal(this.columnIndex,
                                               this.columnWidth_ + deltaX,
                                               true);
};

/**
 * Handles the drag end event.
 */
FileTableSplitter.prototype.handleSplitterDragEnd = function() {
  cr.ui.TableSplitter.prototype.handleSplitterDragEnd.call(this);
  this.table_.columnModel.handleSplitterDragEnd();
};

/**
 * File list Table View.
 * @constructor
 * @extends {cr.ui.Table}
 */
function FileTable() {
  throw new Error('Designed to decorate elements');
}

/**
 * Inherits from cr.ui.Table.
 */
FileTable.prototype.__proto__ = cr.ui.Table.prototype;

/**
 * Decorates the element.
 * @param {!Element} self Table to decorate.
 * @param {MetadataCache} metadataCache To retrieve thumbnails.
 * @param {!FileSystemMetadata} fileSystemMetadata To retrieve
 *     metadata.
 * @param {VolumeManagerWrapper} volumeManager To retrieve volume info.
 * @param {!importer.HistoryLoader} historyLoader
 * @param {boolean} fullPage True if it's full page File Manager,
 *                           False if a file open/save dialog.
 */
FileTable.decorate = function(
    self, metadataCache, fileSystemMetadata, volumeManager, historyLoader,
    fullPage) {
  cr.ui.Table.decorate(self);
  self.__proto__ = FileTable.prototype;
  self.metadataCache_ = metadataCache;
  self.fileSystemMetadata_ = fileSystemMetadata;
  self.volumeManager_ = volumeManager;
  self.historyLoader_ = historyLoader;

  var nameColumn = new cr.ui.table.TableColumn(
      'name', str('NAME_COLUMN_LABEL'), fullPage ? 386 : 324);
  nameColumn.renderFunction = self.renderName_.bind(self);

  var sizeColumn = new cr.ui.table.TableColumn(
      'size', str('SIZE_COLUMN_LABEL'), 110, true);
  sizeColumn.renderFunction = self.renderSize_.bind(self);
  sizeColumn.defaultOrder = 'desc';

  var statusColumn = new cr.ui.table.TableColumn(
      'status', str('STATUS_COLUMN_LABEL'), 50, true);
  statusColumn.renderFunction = self.renderStatus_.bind(self);

  var typeColumn = new cr.ui.table.TableColumn(
      'type', str('TYPE_COLUMN_LABEL'), fullPage ? 110 : 110);
  typeColumn.renderFunction = self.renderType_.bind(self);

  var modTimeColumn = new cr.ui.table.TableColumn(
      'modificationTime', str('DATE_COLUMN_LABEL'), fullPage ? 150 : 210);
  modTimeColumn.renderFunction = self.renderDate_.bind(self);
  modTimeColumn.defaultOrder = 'desc';

  var columns = [
      nameColumn,
      sizeColumn,
      typeColumn,
      modTimeColumn
  ];
  // Display the status column only if cloud imports are enabled.
  if (self.importEnabled) {
    columns.splice(2, 0, statusColumn);
  }

  var columnModel = new FileTableColumnModel(columns);

  self.columnModel = columnModel;
  self.setDateTimeFormat(true);
  self.setRenderFunction(self.renderTableRow_.bind(self,
      self.getRenderFunction()));

  self.scrollBar_ = new MainPanelScrollBar();
  self.scrollBar_.initialize(self, self.list);
  // Keep focus on the file list when clicking on the header.
  self.header.addEventListener('mousedown', function(e) {
    self.list.focus();
    e.preventDefault();
  });

  self.relayoutRateLimiter_ =
      new AsyncUtil.RateLimiter(self.relayoutImmediately_.bind(self));

  // Override header#redraw to use FileTableSplitter.
  /** @this {cr.ui.table.TableHeader} */
  self.header_.redraw = function() {
    this.__proto__.redraw.call(this);
    // Extend table splitters
    var splitters = this.querySelectorAll('.table-header-splitter');
    for (var i = 0; i < splitters.length; i++) {
      if (splitters[i] instanceof FileTableSplitter)
        continue;
      FileTableSplitter.decorate(splitters[i]);
    }
  };

  // Save the last selection. This is used by shouldStartDragSelection.
  self.list.addEventListener('mousedown', function(e) {
    this.lastSelection_ = this.selectionModel.selectedIndexes;
  }.bind(self), true);
  self.list.shouldStartDragSelection =
      self.shouldStartDragSelection_.bind(self);

  /**
   * Obtains the index list of elements that are hit by the point or the
   * rectangle.
   *
   * @param {number} x X coordinate value.
   * @param {number} y Y coordinate value.
   * @param {number=} opt_width Width of the coordinate.
   * @param {number=} opt_height Height of the coordinate.
   * @return {Array.<number>} Index list of hit elements.
   * @this {cr.ui.List}
   */
  self.list.getHitElements = function(x, y, opt_width, opt_height) {
    var currentSelection = [];
    var bottom = y + (opt_height || 0);
    for (var i = 0; i < this.selectionModel_.length; i++) {
      var itemMetrics = this.getHeightsForIndex_(i);
      if (itemMetrics.top < bottom && itemMetrics.top + itemMetrics.height >= y)
        currentSelection.push(i);
    }
    return currentSelection;
  };
};

/**
 * Adjust column width to fit its content.
 * @param {number} index Index of the column to adjust width.
 * @override
 */
FileTable.prototype.fitColumn = function(index) {
  var render = this.columnModel.getRenderFunction(index);
  var MAXIMUM_ROWS_TO_MEASURE = 1000;

  // Create a temporaty list item, put all cells into it and measure its
  // width. Then remove the item. It fits "list > *" CSS rules.
  var container = this.ownerDocument.createElement('li');
  container.style.display = 'inline-block';
  container.style.textAlign = 'start';
  // The container will have width of the longest cell.
  container.style.webkitBoxOrient = 'vertical';

  // Select at most MAXIMUM_ROWS_TO_MEASURE items around visible area.
  var items = this.list.getItemsInViewPort(this.list.scrollTop,
                                           this.list.clientHeight);
  var firstIndex = Math.floor(Math.max(0,
      (items.last + items.first - MAXIMUM_ROWS_TO_MEASURE) / 2));
  var lastIndex = Math.min(this.dataModel.length,
                           firstIndex + MAXIMUM_ROWS_TO_MEASURE);
  for (var i = firstIndex; i < lastIndex; i++) {
    var item = this.dataModel.item(i);
    var div = this.ownerDocument.createElement('div');
    div.className = 'table-row-cell';
    div.appendChild(render(item, this.columnModel.getId(index), this));
    container.appendChild(div);
  }
  this.list.appendChild(container);
  var width = parseFloat(window.getComputedStyle(container).width);
  this.list.removeChild(container);

  this.columnModel.initializeColumnPos();
  this.columnModel.setWidthAndKeepTotal(index, Math.ceil(width));
  this.columnModel.destroyColumnPos();
};

/**
 * Sets date and time format.
 * @param {boolean} use12hourClock True if 12 hours clock, False if 24 hours.
 */
FileTable.prototype.setDateTimeFormat = function(use12hourClock) {
  this.timeFormatter_ = new Intl.DateTimeFormat(
      [] /* default locale */,
      {hour: 'numeric', minute: 'numeric', hour12: use12hourClock});
  this.dateFormatter_ = new Intl.DateTimeFormat(
      [] /* default locale */,
      {
        year: 'numeric', month: 'short', day: 'numeric',
        hour: 'numeric', minute: 'numeric', hour12: use12hourClock
      });
};

/**
 * Obtains if the drag selection should be start or not by referring the mouse
 * event.
 * @param {MouseEvent} event Drag start event.
 * @return {boolean} True if the mouse is hit to the background of the list.
 * @private
 */
FileTable.prototype.shouldStartDragSelection_ = function(event) {
  // If the shift key is pressed, it should starts drag selection.
  if (event.shiftKey)
    return true;

  // If the position values are negative, it points the out of list.
  // It should start the drag selection.
  var pos = DragSelector.getScrolledPosition(this.list, event);
  if (!pos)
    return false;
  if (pos.x < 0 || pos.y < 0)
    return true;

  // If the item index is out of range, it should start the drag selection.
  var itemHeight = this.list.measureItem().height;
  // Faster alternative to Math.floor for non-negative numbers.
  var itemIndex = ~~(pos.y / itemHeight);
  if (itemIndex >= this.list.dataModel.length)
    return true;

  // If the pointed item is already selected, it should not start the drag
  // selection.
  if (this.lastSelection_ && this.lastSelection_.indexOf(itemIndex) !== -1)
    return false;

  // If the horizontal value is not hit to column, it should start the drag
  // selection.
  var hitColumn = this.columnModel.getHitColumn(pos.x);
  if (!hitColumn)
    return true;

  // Check if the point is on the column contents or not.
  var item = this.list.getListItemByIndex(itemIndex);
  switch (this.columnModel.columns_[hitColumn.index].id) {
    case 'name':
      var spanElement = item.querySelector('.filename-label span');
      var spanRect = spanElement.getBoundingClientRect();
      // The this.list.cachedBounds_ object is set by
      // DragSelector.getScrolledPosition.
      if (!this.list.cachedBounds)
        return true;
      var textRight =
          spanRect.left - this.list.cachedBounds.left + spanRect.width;
      return textRight <= hitColumn.hitPosition;
    default:
      return true;
  }
};

/**
 * Render the Name column of the detail table.
 *
 * Invoked by cr.ui.Table when a file needs to be rendered.
 *
 * @param {Entry} entry The Entry object to render.
 * @param {string} columnId The id of the column to be rendered.
 * @param {cr.ui.Table} table The table doing the rendering.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderName_ = function(entry, columnId, table) {
  var label = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));

  var icon = this.renderIconType_(entry, columnId, table);
  if (FileType.isImage(entry) || FileType.isVideo(entry))
    icon.appendChild(this.renderThumbnail_(entry));
  icon.appendChild(this.renderCheckmark_());
  icon.addEventListener(
      'mouseup', util.repeatMouseEventWithCtrlKey.bind(null, label));
  icon.addEventListener(
      'mousedown', util.repeatMouseEventWithCtrlKey.bind(null, label));
  label.appendChild(icon);

  label.entry = entry;
  label.className = 'detail-name';
  label.appendChild(filelist.renderFileNameLabel(this.ownerDocument, entry));
  return label;
};

/**
 * Render the Size column of the detail table.
 *
 * @param {Entry} entry The Entry object to render.
 * @param {string} columnId The id of the column to be rendered.
 * @param {cr.ui.Table} table The table doing the rendering.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderSize_ = function(entry, columnId, table) {
  var div = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));
  div.className = 'size';
  this.updateSize_(div, entry);

  return div;
};

/**
 * Sets up or updates the size cell.
 *
 * @param {HTMLDivElement} div The table cell.
 * @param {Entry} entry The corresponding entry.
 * @private
 */
FileTable.prototype.updateSize_ = function(div, entry) {
  var metadata = this.fileSystemMetadata_.getCache(
      [entry], ['size', 'hosted'])[0];
  var size = metadata.size;
  if (size === null || size === undefined) {
    div.textContent = '...';
  } else if (size === -1) {
    div.textContent = '--';
  } else if (size === 0 && metadata.hosted) {
    div.textContent = '--';
  } else {
    div.textContent = util.bytesToString(size);
  }
};

/**
 * Render the Status column of the detail table.
 *
 * @param {Entry} entry The Entry object to render.
 * @param {string} columnId The id of the column to be rendered.
 * @param {cr.ui.Table} table The table doing the rendering.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderStatus_ = function(entry, columnId, table) {
  var div = /** @type {!HTMLDivElement} */ (
      this.ownerDocument.createElement('div'));
  div.className = 'status status-icon';
  if (entry) {
    this.updateStatus_(div, entry);
  }

  return div;
};

/**
 * Returns the status of the entry w.r.t. the given import destination.
 * @param {Entry} entry
 * @param {!importer.Destination} destination
 * @return {!Promise<string>} The import status - will be 'imported', 'copied',
 *     or 'unknown'.
 */
FileTable.prototype.getImportStatus_ = function(entry, destination) {
  if (!importer.isEligibleEntry(this.volumeManager_, entry)) {
    // Our import history doesn't deal with directories.
    // TODO(kenobi): May need to revisit this if the above assumption changes.
    return Promise.resolve('unknown');
  }
  // For the compiler.
  var fileEntry = /** @type {!FileEntry} */ (entry);

  return this.historyLoader_.getHistory()
      .then(
          /** @param {!importer.ImportHistory} history */
          function(history) {
            return Promise.all([
                history.wasImported(fileEntry, destination),
                history.wasCopied(fileEntry, destination)
            ]);
          })
      .then(
          /** @param {!Array<boolean>} status */
          function(status) {
            if (status[0]) {
              return 'imported';
            } else if (status[1]) {
              return 'copied';
            } else {
              return 'unknown';
            }
          });
};

/**
 * Render the status icon of the detail table.
 *
 * @param {HTMLDivElement} div
 * @param {Entry} entry The Entry object to render.
 * @private
 */
FileTable.prototype.updateStatus_ = function(div, entry) {
  this.getImportStatus_(entry, importer.Destination.GOOGLE_DRIVE).then(
      /** @param {string} status */
      function(status) {
        div.setAttribute('file-status-icon', status);
      });
};

/**
 * Render the Type column of the detail table.
 *
 * @param {Entry} entry The Entry object to render.
 * @param {string} columnId The id of the column to be rendered.
 * @param {cr.ui.Table} table The table doing the rendering.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderType_ = function(entry, columnId, table) {
  var div = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));
  div.className = 'type';
  div.textContent = FileType.typeToString(FileType.getType(entry));
  return div;
};

/**
 * Render the Date column of the detail table.
 *
 * @param {Entry} entry The Entry object to render.
 * @param {string} columnId The id of the column to be rendered.
 * @param {cr.ui.Table} table The table doing the rendering.
 * @return {HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderDate_ = function(entry, columnId, table) {
  var div = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));
  div.className = 'date';

  this.updateDate_(div, entry);
  return div;
};

/**
 * Sets up or updates the date cell.
 *
 * @param {HTMLDivElement} div The table cell.
 * @param {Entry} entry Entry of file to update.
 * @private
 */
FileTable.prototype.updateDate_ = function(div, entry) {
  var modTime = this.fileSystemMetadata_.getCache(
      [entry], ['modificationTime'])[0].modificationTime;

  if (!modTime) {
    div.textContent = '...';
    return;
  }

  var today = new Date();
  today.setHours(0);
  today.setMinutes(0);
  today.setSeconds(0);
  today.setMilliseconds(0);

  /**
   * Number of milliseconds in a day.
   */
  var MILLISECONDS_IN_DAY = 24 * 60 * 60 * 1000;

  if (isNaN(modTime.getTime())) {
    // In case of 'Invalid Date'.
    div.textContent = '--';
  } else if (modTime >= today &&
      modTime < today.getTime() + MILLISECONDS_IN_DAY) {
    div.textContent = strf('TIME_TODAY', this.timeFormatter_.format(modTime));
  } else if (modTime >= today - MILLISECONDS_IN_DAY && modTime < today) {
    div.textContent = strf('TIME_YESTERDAY',
                           this.timeFormatter_.format(modTime));
  } else {
    div.textContent = this.dateFormatter_.format(modTime);
  }
};

/**
 * Updates the file metadata in the table item.
 *
 * @param {Element} item Table item.
 * @param {Entry} entry File entry.
 */
FileTable.prototype.updateFileMetadata = function(item, entry) {
  this.updateDate_(
      /** @type {!HTMLDivElement} */ (item.querySelector('.date')), entry);
  this.updateSize_(
      /** @type {!HTMLDivElement} */ (item.querySelector('.size')), entry);
  this.updateStatus_(
      /** @type {!HTMLDivElement} */ (item.querySelector('.status')), entry);
};

/**
 * Updates list items 'in place' on metadata change.
 * @param {string} type Type of metadata change.
 * @param {Array.<Entry>} entries Entries to update.
 */
FileTable.prototype.updateListItemsMetadata = function(type, entries) {
  var urls = util.entriesToURLs(entries);
  var forEachCell = function(selector, callback) {
    var cells = this.querySelectorAll(selector);
    for (var i = 0; i < cells.length; i++) {
      var cell = cells[i];
      var listItem = this.list_.getListItemAncestor(cell);
      var entry = this.dataModel.item(listItem.listIndex);
      if (entry && urls.indexOf(entry.toURL()) !== -1)
        callback.call(this, cell, entry, listItem);
    }
  }.bind(this);
  if (type === 'filesystem') {
    forEachCell('.table-row-cell > .date', function(item, entry, unused) {
      this.updateDate_(item, entry);
    });
    forEachCell('.table-row-cell > .size', function(item, entry, unused) {
      this.updateSize_(item, entry);
    });
  } else if (type === 'external') {
    // The cell name does not matter as the entire list item is needed.
    forEachCell('.table-row-cell > .date', function(item, entry, listItem) {
      filelist.updateListItemExternalProps(
          listItem,
          this.fileSystemMetadata_.getCache(
              [entry], ['availableOffline', 'customIconUrl'])[0]);
    });
  } else if (type === 'import-history') {
    forEachCell('.table-row-cell > .status', function(item, entry, unused) {
      this.updateStatus_(item, entry);
    });
  }
};

/**
 * Renders table row.
 * @param {function(Entry, cr.ui.Table)} baseRenderFunction Base renderer.
 * @param {Entry} entry Corresponding entry.
 * @return {HTMLLIElement} Created element.
 * @private
 */
FileTable.prototype.renderTableRow_ = function(baseRenderFunction, entry) {
  var item = baseRenderFunction(entry, this);
  filelist.decorateListItem(item, entry, this.fileSystemMetadata_);
  return item;
};

/**
 * Render the type column of the detail table.
 *
 * Invoked by cr.ui.Table when a file needs to be rendered.
 *
 * @param {Entry} entry The Entry object to render.
 * @param {string} columnId The id of the column to be rendered.
 * @param {cr.ui.Table} table The table doing the rendering.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderIconType_ = function(entry, columnId, table) {
  var icon = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));
  icon.className = 'detail-icon';
  icon.setAttribute('file-type-icon', FileType.getIcon(entry));
  return icon;
};

/**
 * Renders the file thumbnail in the detail table.
 * @param {Entry} entry The Entry object to render.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderThumbnail_ = function(entry) {
  var box = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));
  box.className = 'detail-thumbnail';

  if (entry) {
    this.metadataCache_.getOne(
        entry, 'thumbnail|filesystem|external|media',
        function(metadata) {
          var loader = new ThumbnailLoader(
              entry, ThumbnailLoader.LoaderType.IMAGE, metadata);
          loader.load(box, ThumbnailLoader.FillMode.OVER_FILL,
                      ThumbnailLoader.OptimizationMode.DISCARD_DETACHED,
                      function(image, transform) {
                        box.classList.add('loaded');
                      });
        });
  }
  return box;
};

/**
 * Renders the selection checkmark in the detail table.
 * @return {!HTMLDivElement} Created element.
 * @private
 */
FileTable.prototype.renderCheckmark_ = function() {
  var checkmark = /** @type {!HTMLDivElement} */
      (this.ownerDocument.createElement('div'));
  checkmark.className = 'detail-checkmark';
  return checkmark;
};

/**
 * Sets the margin height for the transparent preview panel at the bottom.
 * @param {number} margin Margin to be set in px.
 */
FileTable.prototype.setBottomMarginForPanel = function(margin) {
  this.list_.style.paddingBottom = margin + 'px';
  this.scrollBar_.setBottomMarginForPanel(margin);
};

/**
 * Redraws the UI. Skips multiple consecutive calls.
 */
FileTable.prototype.relayout = function() {
  this.relayoutRateLimiter_.run();
};

/**
 * Redraws the UI immediately.
 * @private
 */
FileTable.prototype.relayoutImmediately_ = function() {
  if (this.clientWidth > 0)
    this.normalizeColumns();
  this.redraw();
  cr.dispatchSimpleEvent(this.list, 'relayout');
};

/**
 * Common item decoration for table's and grid's items.
 * @param {cr.ui.ListItem} li List item.
 * @param {Entry} entry The entry.
 * @param {!FileSystemMetadata} fileSystemMetadata Cache to
 *     retrieve metadada.
 */
filelist.decorateListItem = function(li, entry, fileSystemMetadata) {
  li.classList.add(entry.isDirectory ? 'directory' : 'file');
  // The metadata may not yet be ready. In that case, the list item will be
  // updated when the metadata is ready via updateListItemsMetadata. For files
  // not on an external backend, externalProps is not available.
  var externalProps = fileSystemMetadata.getCache(
      [entry], ['hosted', 'availableOffline'])[0];
  filelist.updateListItemExternalProps(li, externalProps);

  // Overriding the default role 'list' to 'listbox' for better
  // accessibility on ChromeOS.
  li.setAttribute('role', 'option');

  Object.defineProperty(li, 'selected', {
    /**
     * @this {cr.ui.ListItem}
     * @return {boolean} True if the list item is selected.
     */
    get: function() {
      return this.hasAttribute('selected');
    },

    /**
     * @this {cr.ui.ListItem}
     */
    set: function(v) {
      if (v)
        this.setAttribute('selected', '');
      else
        this.removeAttribute('selected');
    }
  });
};

/**
 * Render filename label for grid and list view.
 * @param {Document} doc Owner document.
 * @param {Entry} entry The Entry object to render.
 * @return {!HTMLDivElement} The label.
 */
filelist.renderFileNameLabel = function(doc, entry) {
  // Filename need to be in a '.filename-label' container for correct
  // work of inplace renaming.
  var box = /** @type {!HTMLDivElement} */ (doc.createElement('div'));
  box.className = 'filename-label';
  var fileName = doc.createElement('span');
  fileName.className = 'entry-name';
  fileName.textContent = entry.name;
  box.appendChild(fileName);

  return box;
};

/**
 * Updates grid item or table row for the externalProps.
 * @param {cr.ui.ListItem} li List item.
 * @param {Object} externalProps Metadata.
 */
filelist.updateListItemExternalProps = function(li, externalProps) {
  if (li.classList.contains('file')) {
    if (externalProps.availableOffline)
      li.classList.remove('dim-offline');
    else
      li.classList.add('dim-offline');
    // TODO(mtomasz): Consider adding some vidual indication for files which
    // are not cached on LTE. Currently we show them as normal files.
    // crbug.com/246611.
  }

  var iconDiv = li.querySelector('.detail-icon');
  if (!iconDiv)
    return;

  if (externalProps.customIconUrl)
    iconDiv.style.backgroundImage = 'url(' + externalProps.customIconUrl + ')';
  else
    iconDiv.style.backgroundImage = '';  // Back to the default image.

  if (li.classList.contains('directory'))
    iconDiv.classList.toggle('shared', externalProps.shared);
};
