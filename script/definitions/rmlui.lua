---@meta

---@class Vector2i
Vector2i = {}

---@param x number
---@param y number
---@return Vector2i
function Vector2i.new(x, y) end

---@class rmlui
rmlui = {}

---@param name "play"
---@param size Vector2i
---@return Context
function rmlui:CreateContext(name, size) end

--- RmlUi::Context
---@class Context
Context = {}

---@param path string
---@return Document
function Context:LoadDocument(path) end

function Context:Update() end

function Context:Render() end

---@class Document
Document = {}

function Document:Show() end
