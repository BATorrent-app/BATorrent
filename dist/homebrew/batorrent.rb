cask "batorrent" do
  version "2.5.1"
  sha256 "639798814ad7236f0e989458bb0811033e7999aae581be85ef746faeea7fc36b"

  url "https://github.com/Mateuscruz19/BATorrent/releases/download/v#{version}/BATorrent-macos-arm64.dmg"
  name "BATorrent"
  desc "Open-source BitTorrent client with VPN kill switch, anti-Xunlei, Tor, and 7 languages"
  homepage "https://github.com/Mateuscruz19/BATorrent"

  livecheck do
    url :url
    strategy :github_latest
  end

  depends_on macos: ">= :monterey"
  depends_on arch: :arm64

  app "BATorrent.app"

  # Ad-hoc codesigned (open-source project without Apple Developer ID).
  # Without this, Gatekeeper quarantines the app on first launch.
  caveats <<~EOS
    #{token} is ad-hoc signed. On first launch macOS may show a security warning.
    To allow it: System Settings → Privacy & Security → Open Anyway.
  EOS

  zap trash: [
    "~/Library/Application Support/BATorrent",
    "~/Library/Preferences/com.batorrent.app.plist",
    "~/Library/Saved Application State/com.batorrent.app.savedState",
  ]
end
