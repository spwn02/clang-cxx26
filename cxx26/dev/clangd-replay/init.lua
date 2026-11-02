-- Headless Neovim init for replaying clangd behaviour through the real editor
-- stack (nvim-lspconfig + blink.cmp). Configured by environment variables:
--   REPLAY_CLANGD   clangd binary to test (required)
--   REPLAY_LAZY     directory holding the lazy.nvim plugins (default ~/.local/share/nvim/lazy)
vim.opt.rtp:prepend(vim.fn.expand((vim.env.REPLAY_LAZY or "~/.local/share/nvim/lazy") .. "/nvim-lspconfig"))
vim.opt.rtp:prepend(vim.fn.expand((vim.env.REPLAY_LAZY or "~/.local/share/nvim/lazy") .. "/blink.cmp"))
vim.opt.swapfile = false
require("blink.cmp").setup({
  fuzzy = { implementation = "prefer_rust_with_warning" },
  completion = { menu = { auto_show = true } },
})
vim.lsp.config("clangd", {
  cmd = { vim.env.REPLAY_CLANGD, "--background-index=false", "--completion-style=detailed",
          "--header-insertion=" .. (vim.env.REPLAY_HEADER_INSERTION or "never"), "--log=error" },
  root_markers = { "compile_commands.json" },
  filetypes = { "cpp" },
  capabilities = require("blink.cmp").get_lsp_capabilities(),
})
vim.lsp.enable("clangd")
