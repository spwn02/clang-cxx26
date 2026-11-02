local out = {}
local function log(s) table.insert(out, s) end
local function finish() vim.fn.writefile(out, vim.env.REPLAY_OUT); vim.cmd("qa!") end
local function items(label)
  local ok, its = pcall(function() return require("blink.cmp").get_items() end)
  local names = {}
  if ok and its then for _, it in ipairs(its) do if it.source_id == "lsp" then table.insert(names, it.label) end end end
  log(label .. " [" .. #names .. " lsp items]: " .. table.concat(vim.list_slice(names, 1, 14), " | "))
end
local steps = vim.env.REPLAY_STEPS and vim.json.decode(vim.env.REPLAY_STEPS) or {}
vim.cmd("edit " .. vim.env.REPLAY_FILE)
local attached = false
vim.api.nvim_create_autocmd("DiagnosticChanged", { once = true, callback = function() attached = true end })
local function run_step(i)
  local s = steps[i]
  if not s then return finish() end
  if s.line then vim.api.nvim_win_set_cursor(0, { s.line, s.col or 0 }) end
  if s.input then vim.api.nvim_input(s.input) end
  vim.defer_fn(function()
    if s.show then require("blink.cmp").show() end
    vim.defer_fn(function()
      if s.report then items(s.report) end
      if s.request then
        -- {"request": "textDocument/hover|references|documentHighlight", "report": label}: the answer of the server
        local pos = vim.lsp.util.make_position_params(0, "utf-16")
        local params = { textDocument = pos.textDocument, position = pos.position, context = { includeDeclaration = true } }
        local res = vim.lsp.buf_request_sync(0, s.request, params, 5000) or {}
        local parts = {}
        for _, r in pairs(res) do
          local v = r.result
          if type(v) == "table" and v.contents then
            local c = v.contents
            local txt = (type(c) == "table" and (c.value or vim.inspect(c)) or tostring(c)):gsub("%s+", " ")
            table.insert(parts, txt)
          elseif type(v) == "table" then
            table.insert(parts, #v .. " results")
            for _, x in ipairs(v) do
              local rg = x.range or (x.location and x.location.range)
              if rg then table.insert(parts, rg.start.line + 1 .. ":" .. rg.start.character + 1) end
            end
          end
        end
        log((s.label or s.request) .. " [lsp request]: " .. table.concat(parts, " | "))
      end
      run_step(i + 1)
    end, s.wait or 1500)
  end, s.pre or 300)
end
local tries = 0
local function wait_attach()
  tries = tries + 1
  if attached or tries > 60 then log("attached=" .. tostring(attached) .. " clients=" .. #vim.lsp.get_clients()); return run_step(1) end
  vim.defer_fn(wait_attach, 500)
end
vim.defer_fn(wait_attach, 500)
