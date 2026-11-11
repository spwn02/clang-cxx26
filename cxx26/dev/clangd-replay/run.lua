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
      if s.dump then
        -- {"dump": label}: the last lines of the buffer, to check what the typed keys produced
        local n = vim.api.nvim_buf_line_count(0)
        log(s.dump .. " [buffer, " .. n .. " lines]: " .. table.concat(vim.api.nvim_buf_get_lines(0, math.max(0, n - 4), n, false), " ⏎ "))
      end
      if s.diagnostics then
        -- {"diagnostics": label}: the diagnostics of the buffer as "severity:line:message"
        local ds, parts = vim.diagnostic.get(0), {}
        table.insert(parts, #ds .. " diagnostics")
        for _, d in ipairs(ds) do table.insert(parts, d.severity .. ":" .. (d.lnum + 1) .. ":" .. (d.message:gsub("%s+", " "))) end
        log(s.diagnostics .. " [diagnostics]: " .. table.concat(parts, " | "))
      end
      if s.request then
        -- {"request": "textDocument/hover|references|documentHighlight", "report": label}: the answer of the server
        local pos = vim.lsp.util.make_position_params(0, "utf-16")
        local params = { textDocument = pos.textDocument, position = pos.position, context = { includeDeclaration = true, triggerKind = 1 } }
        if s.request == "textDocument/rename" then params.newName = s.newName or "renamed" end
        local res = vim.lsp.buf_request_sync(0, s.request, params, 5000) or {}
        if s.request == "textDocument/completion" then
          -- {"match": "<label>"}: print the item with that label and the text its additionalTextEdits insert
          local parts = {}
          for _, r in pairs(res) do
            local list = r.result and (r.result.items or r.result) or {}
            for _, it in ipairs(list) do
              if it.label and it.label:find(s.match, 1, true) then
                local edits = {}
                for _, e in ipairs(it.additionalTextEdits or {}) do table.insert(edits, (e.newText:gsub("%s+", " "))) end
                table.insert(parts, it.label:gsub("^%s+", "") .. " edits=" .. table.concat(edits, ";"))
              end
            end
          end
          local n = 0; for _, r in pairs(res) do if r.err then log("err: " .. vim.inspect(r.err)) end end
          for _, r in pairs(res) do n = n + #(r.result and (r.result.items or r.result) or {}) end
          log((s.label or s.request) .. " [lsp request, " .. n .. " items]: " .. table.concat(parts, " | "))
          return run_step(i + 1)
        end
        local parts = {}
        for _, r in pairs(res) do
          local v = r.result
          if s.request == "textDocument/rename" and type(v) == "table" then
            -- the edits as line:col of each replaced range
            local chg = v.changes or {}
            for _, dc in ipairs(v.documentChanges or {}) do chg[dc.textDocument.uri] = dc.edits end
            for _, edits in pairs(chg) do
              table.insert(parts, #edits .. " edits")
              for _, e in ipairs(edits) do table.insert(parts, e.range.start.line + 1 .. ":" .. e.range.start.character + 1) end
            end
          elseif s.request == "textDocument/semanticTokens/full" then
            -- the tokens as line:col:len (delta encoding decoded)
            local d, line, col = (v and v.data) or {}, 0, 0
            table.insert(parts, #d / 5 .. " tokens")
            for k = 1, #d, 5 do
              if d[k] > 0 then line = line + d[k]; col = d[k + 1] else col = col + d[k + 1] end
              table.insert(parts, line + 1 .. ":" .. col + 1 .. ":" .. d[k + 2])
            end
          elseif s.request == "textDocument/foldingRange" and type(v) == "table" then
            table.insert(parts, #v .. " ranges")
            for _, x in ipairs(v) do table.insert(parts, x.startLine + 1 .. "-" .. x.endLine + 1) end
          elseif type(v) == "table" and v.contents then
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
