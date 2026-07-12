class Controllable {
  public:
    virtual void start( unsigned ) = 0;
    virtual void stop( unsigned ) = 0;
    virtual void pause( unsigned ) = 0;
    virtual void unpause( unsigned ) = 0;
};
