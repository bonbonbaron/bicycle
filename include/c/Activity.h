class Activity {
  public:
    static auto getInstance() -> Activity&;
    void run();
  private:
    Activity();
    Activity(const Activity&) = delete;
    Activity operator=(const Activity&) = delete;
    Activity(const Activity&&) = delete;
    Activity operator=(const Activity&&) = delete;
};  // class Activity
